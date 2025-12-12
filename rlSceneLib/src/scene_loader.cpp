#include "scene_loader.h"

#include "raylib.h"
#include "external/cgltf.h"

#include <unordered_map>

ResolveTextureCallback TextureResolver = nullptr;

std::string_view SceneFileName;

void SetTextureResolver(ResolveTextureCallback resolver)
{
    TextureResolver = resolver;
}

// Load image from different glTF provided methods (uri, path, buffer_view)
static Image LoadImageFromCgltfImage(cgltf_image* cgltfImage, const char* texPath)
{
    Image image = { 0 };

    if (cgltfImage == NULL)
        return GenImageChecked(32, 32, 16, 16, GRAY, LIGHTGRAY);

    if ((cgltfImage->buffer_view != NULL) && (cgltfImage->buffer_view->buffer->data != NULL))    // Check if image is provided as data buffer
    {
        unsigned char* data = (unsigned char*)MemAlloc(cgltfImage->buffer_view->size);
        int offset = (int)cgltfImage->buffer_view->offset;
        int stride = (int)cgltfImage->buffer_view->stride ? (int)cgltfImage->buffer_view->stride : 1;

        // Copy buffer data to memory for loading
        for (unsigned int i = 0; i < cgltfImage->buffer_view->size; i++)
        {
            data[i] = ((unsigned char*)cgltfImage->buffer_view->buffer->data)[offset];
            offset += stride;
        }

        // Check mime_type for image: (cgltfImage->mime_type == "image/png")
        // NOTE: Detected that some models define mime_type as "image\\/png"
        if ((strcmp(cgltfImage->mime_type, "image\\/png") == 0) || (strcmp(cgltfImage->mime_type, "image/png") == 0))
        {
            image = LoadImageFromMemory(".png", data, (int)cgltfImage->buffer_view->size);
        }
        else if ((strcmp(cgltfImage->mime_type, "image\\/jpeg") == 0) || (strcmp(cgltfImage->mime_type, "image/jpeg") == 0))
        {
            image = LoadImageFromMemory(".jpg", data, (int)cgltfImage->buffer_view->size);
        }
        else
        {
            TraceLog(LOG_WARNING, "MODEL: glTF image data MIME type not recognized", TextFormat("%s/%s", texPath, cgltfImage->uri));
        }

        MemFree(data);
    }

    return image;
}


#define LOAD_ATTRIBUTE(accesor, numComp, srcType, dstPtr) LOAD_ATTRIBUTE_CAST(accesor, numComp, srcType, dstPtr, srcType)

#define LOAD_ATTRIBUTE_CAST(accesor, numComp, srcType, dstPtr, dstType) \
    { \
        int n = 0; \
        srcType *buffer = (srcType *)accesor->buffer_view->buffer->data + accesor->buffer_view->offset/sizeof(srcType) + accesor->offset/sizeof(srcType); \
        for (unsigned int k = 0; k < accesor->count; k++) \
        {\
            for (int l = 0; l < numComp; l++) \
            {\
                dstPtr[numComp*k + l] = (dstType)buffer[n + l];\
            }\
            n += (int)(accesor->stride/sizeof(srcType));\
        }\
    }

// Load file data callback for cgltf
static cgltf_result LoadFileGLTFCallback(const struct cgltf_memory_options* memoryOptions, const struct cgltf_file_options* fileOptions, const char* path, cgltf_size* size, void** data)
{
    int filesize;
    unsigned char* filedata = LoadFileData(path, &filesize);

    if (filedata == NULL) return cgltf_result_io_error;

    *size = filesize;
    *data = filedata;

    return cgltf_result_success;
}

// Release file data callback for cgltf
static void ReleaseFileGLTFCallback(const struct cgltf_memory_options* memoryOptions, const struct cgltf_file_options* fileOptions, void* data)
{
    UnloadFileData((unsigned char*)data);
}

std::size_t GetAttributeBufferHash(cgltf_accessor* accesor)
{
    std::size_t hash = 2166136261U; // FNV_offset_basis

    int n = 0;
    unsigned char* buffer = (unsigned char*)accesor->buffer_view->buffer->data + accesor->buffer_view->offset / sizeof(unsigned char) + accesor->offset / sizeof(unsigned char);
    for (unsigned int k = 0; k < accesor->count; k++)
    {
        for (int l = 0; l < 1; l++)
        {
            hash ^= (unsigned char)buffer[n + l];
            hash *= 16777619U; // FNV_prime
        }
        n += (int)(accesor->stride / sizeof(unsigned char));
    }
    return hash;
}

size_t GetMeshHash(cgltf_primitive* primitive)
{
    size_t hash = 0;

    for (size_t j = 0; j < primitive->attributes_count; j++)
    {
        cgltf_attribute* attribute = &primitive->attributes[j];
        hash ^= GetAttributeBufferHash(attribute->data);
    }

    if (primitive->indices)
    {
        hash ^= GetAttributeBufferHash(primitive->indices);
    }

    return hash;
}

template<class T, class F>
void CopyBufferType(F* outBuffer, cgltf_accessor* data, int componentCount)
{
    T* temp = (T*)MemAlloc(int(data->count * componentCount * sizeof(T)));
    LOAD_ATTRIBUTE(data, componentCount, T, temp);

    for (size_t t = 0; t < data->count * componentCount; t++)
        outBuffer[t] = (F)temp[t];

    RL_FREE(temp);
}

template<class T>
void CopyBufferFloat(float* outBuffer, cgltf_accessor* data, int componentCount)
{
    CopyBufferFloat<T, float>(outBuffer, data, componentCount);
}

template<class T>
bool ConvertBufferType(T* outBuffer, cgltf_accessor* data, int componentCount, float scale = 1.0f)
{
    switch (data->component_type)
    {
    case cgltf_component_type_r_8u:
        CopyBufferType<uint8_t, T>(outBuffer, data, componentCount);
        break;

    case cgltf_component_type_r_8:
        CopyBufferType<int8_t, T>(outBuffer, data, componentCount);
        break;

    case cgltf_component_type_r_16u:
        CopyBufferType<uint16_t, T>(outBuffer, data, componentCount);
        break;

    case cgltf_component_type_r_16:
        CopyBufferType<int16_t, T>(outBuffer, data, componentCount);
        break;

    case cgltf_component_type_r_32u:
        CopyBufferType<uint32_t, T>(outBuffer, data, componentCount);
        break;

    case cgltf_component_type_r_32f:
        CopyBufferType<float, T>(outBuffer, data, componentCount);
        break;

    default:
        return false;
    }

    if (scale != 1.0f)
    {
        for (size_t i = 0; i < data->count * componentCount; i++)
        {
            outBuffer[i] = T(outBuffer[i] * scale);
        }
    }

    return true;
}

std::shared_ptr<Mesh> CacheMesh(Scene& outScene, size_t hash, cgltf_primitive* primitive)
{
    std::shared_ptr<Mesh> newMesh = std::make_shared<Mesh>();

    memset(newMesh.get(), 0, sizeof(Mesh));

    for (size_t i = 0; i < primitive->attributes_count; i++)
    {
        cgltf_attribute* attribute = &primitive->attributes[i];

        switch (attribute->type)
        {
        case cgltf_attribute_type_position:
            newMesh->vertexCount = (int)attribute->data->count;
            newMesh->vertices = (float*)MemAlloc(newMesh->vertexCount * 3 * sizeof(float));
            ConvertBufferType<float>(newMesh->vertices, attribute->data, 3);
            break;

        case cgltf_attribute_type_normal:
            newMesh->normals = (float*)MemAlloc((int)attribute->data->count * 3 * sizeof(float));
            ConvertBufferType<float>(newMesh->normals, attribute->data, 3);
            break;

        case cgltf_attribute_type_texcoord:
        {
            float* ptr = (float*)MemAlloc((int)attribute->data->count * 2 * sizeof(float));
            ConvertBufferType<float>(ptr, attribute->data, 2);

            if (attribute->index == 1)
                newMesh->texcoords2 = ptr;
            else
                newMesh->texcoords = ptr;
        }
        break;
        }
    }

    newMesh->triangleCount = newMesh->vertexCount / 3;

    if (primitive->indices && primitive->indices->buffer_view)
    {
        if (primitive->indices->count > std::numeric_limits<uint16_t>::max())
        {
            // todo. de-index the vertex data
        }
        else
        {
            newMesh->indices = (uint16_t*)MemAlloc((int)primitive->indices->count * sizeof(uint16_t));

            ConvertBufferType<uint16_t>(newMesh->indices, primitive->indices, 1);

            newMesh->triangleCount = int(primitive->indices->count / 3);
        }
    }

    outScene.MeshCache.insert_or_assign(hash, newMesh);

    return newMesh;
}

void LoadMaterial(Material& material, const cgltf_material& gltf_mat)
{
    //	const char* texPath = GetDirectoryPath(fileName);

        // Check glTF material flow: PBR metallic/roughness flow
        // NOTE: Alternatively, materials can follow PBR specular/glossiness flow
    if (gltf_mat.has_pbr_metallic_roughness)
    {
        material.maps[MATERIAL_MAP_ALBEDO].color.r = (unsigned char)(gltf_mat.pbr_metallic_roughness.base_color_factor[0] * 255);
        material.maps[MATERIAL_MAP_ALBEDO].color.g = (unsigned char)(gltf_mat.pbr_metallic_roughness.base_color_factor[1] * 255);
        material.maps[MATERIAL_MAP_ALBEDO].color.b = (unsigned char)(gltf_mat.pbr_metallic_roughness.base_color_factor[2] * 255);
        material.maps[MATERIAL_MAP_ALBEDO].color.a = (unsigned char)(gltf_mat.pbr_metallic_roughness.base_color_factor[3] * 255);

        if (gltf_mat.pbr_metallic_roughness.base_color_texture.texture)
        {
            Image imAlbedo = LoadImageFromCgltfImage(gltf_mat.pbr_metallic_roughness.base_color_texture.texture->image, "");
            if (imAlbedo.data != NULL)
            {
                material.maps[MATERIAL_MAP_ALBEDO].texture = LoadTextureFromImage(imAlbedo);
                UnloadImage(imAlbedo);
            }
        }
    }

    // Other possible materials not supported by raylib pipeline:
    // has_clearcoat, has_transmission, has_volume, has_ior, has specular, has_sheen
}

BoundingBox MergeBoundingBoxes(const BoundingBox& b1, const BoundingBox& b2)
{
    BoundingBox result;
    result.min.x = fminf(b1.min.x, b2.min.x);
    result.min.y = fminf(b1.min.y, b2.min.y);
    result.min.z = fminf(b1.min.z, b2.min.z);
    result.max.x = fmaxf(b1.max.x, b2.max.x);
    result.max.y = fmaxf(b1.max.y, b2.max.y);
    result.max.z = fmaxf(b1.max.z, b2.max.z);
    return result;
}

void LoadMesh(MeshSceneObject* mesh, cgltf_node* node, const cgltf_data* data, Scene& outScene)
{
    for (size_t i = 0; i < node->mesh->primitives_count; i++)
    {
        auto* prim = node->mesh->primitives + i;

        if (prim->attributes_count == 0)
            continue;

        size_t meshHash = GetMeshHash(prim);

        MeshSceneObject::MeshInstanceData meshInstance;
        // read the material?

        meshInstance.MaterialData = LoadMaterialDefault();
        if (prim->material)
        {
            LoadMaterial(meshInstance.MaterialData, *prim->material);
            //prim->material->has_pbr_metallic_roughness;
        }

        auto itr = outScene.MeshCache.find(meshHash);
        if (itr != outScene.MeshCache.end())
        {
            meshInstance.MeshData = itr->second;
        }
        else
        {
            meshInstance.MeshData = CacheMesh(outScene, meshHash, prim);
        }

        auto bbox = GetMeshBoundingBox(*meshInstance.MeshData);

        if (mesh->Meshes.empty())
            mesh->Bounds = bbox;
        else
            mesh->Bounds = MergeBoundingBoxes(bbox, mesh->Bounds);

        mesh->Meshes.push_back(meshInstance);
    }
}

std::unique_ptr<SceneObject> LoadNodeGLTF(cgltf_node* node, const cgltf_data* data, Scene& outScene)
{
    bool storeTransform = true;

    std::unique_ptr<SceneObject> sceneNode = nullptr;
    if (node->camera)
    {
        sceneNode = std::make_unique<CameraSceneObject>();
        CameraSceneObject* camera = static_cast<CameraSceneObject*>(sceneNode.get());

        camera->FOV = RAD2DEG * node->camera->data.perspective.yfov;
        outScene.Cameras.push_back(camera);
    }
    else if (node->light)
    {
        sceneNode = std::make_unique<LightSceneObject>();
        LightSceneObject* light = static_cast<LightSceneObject*>(sceneNode.get());

        light->EmissiveColor = Color{ (unsigned char)(node->light->color[0] * 255), (unsigned char)(node->light->color[1] * 255), (unsigned char)(node->light->color[2] * 255), 255 };
        light->Intensity = 1.0f;// node->light->intensity;

        switch (node->light->type)
        {
        case cgltf_light_type_point:
            light->LightType = LightSceneObject::LightTypes::Point;
            light->Range = node->light->range;
            break;
        case cgltf_light_type_directional:
            light->LightType = LightSceneObject::LightTypes::Directional;
            break;
        case cgltf_light_type_spot:
            light->LightType = LightSceneObject::LightTypes::Spot;
            light->Range = node->light->range;
            if (light->Range == 0)
                light->Range = 20;

            light->MinCone = node->light->spot_inner_cone_angle;
            light->MaxCone = node->light->spot_outer_cone_angle;
            break;
        }

        outScene.Lights.push_back(light);
    }
    else if (node->mesh)
    {
        sceneNode = std::make_unique<MeshSceneObject>();
        MeshSceneObject* mesh = static_cast<MeshSceneObject*>(sceneNode.get());

        LoadMesh(mesh, node, data, outScene);

        outScene.Meshes.push_back(mesh);
    }
    else
    {
        sceneNode = std::make_unique<SceneObject>();
    }

    sceneNode->Name = node->name ? node->name : "";

    sceneNode->Transform.position = Vector3{ node->translation[0], node->translation[1], node->translation[2] };
    sceneNode->Transform.rotation = Quaternion{ node->rotation[0], node->rotation[1], node->rotation[2], node->rotation[3] };
    sceneNode->Transform.scale = Vector3{ node->scale[0], node->scale[1], node->scale[2] };

    cgltf_float worldTransform[16];
    cgltf_node_transform_world(node, worldTransform);

    sceneNode->WorldMatrix = {
        worldTransform[0], worldTransform[4], worldTransform[8], worldTransform[12],
        worldTransform[1], worldTransform[5], worldTransform[9], worldTransform[13],
        worldTransform[2], worldTransform[6], worldTransform[10], worldTransform[14],
        worldTransform[3], worldTransform[7], worldTransform[11], worldTransform[15]
    };

    for (size_t i = 0; i < node->children_count; i++)
    {
        std::unique_ptr<SceneObject> childNode = LoadNodeGLTF(node->children[i], data, outScene);
        childNode->Parent = sceneNode.get();
        sceneNode->Children.push_back(std::move(childNode));
    }

    return sceneNode;
}

bool LoadSceneFromGLTF(std::string_view filename, Scene& outScene)
{
    SceneFileName = filename;

    // glTF file loading
    int dataSize = 0;
    unsigned char* fileData = LoadFileData(filename.data(), &dataSize);

    if (fileData == nullptr)
        return false;

    // glTF data loading
    cgltf_options options = {};
    options.file.read = LoadFileGLTFCallback;
    options.file.release = ReleaseFileGLTFCallback;
    cgltf_data* data = nullptr;
    cgltf_result result = cgltf_parse(&options, fileData, dataSize, &data);

    if (result == cgltf_result_success)
    {
        // Force reading data buffers (fills buffer_view->buffer->data)
        // NOTE: If an uri is defined to base64 data or external path, it's automatically loaded
        result = cgltf_load_buffers(&options, data, filename.data());
        if (result == cgltf_result_success)
        {
            for (size_t i = 0; i < data->scene->nodes_count; i++)
            {
                outScene.RootObjects.emplace_back(std::move(LoadNodeGLTF(data->scene->nodes[i], data, outScene)));
            }
        }

        // Free all cgltf loaded data
        cgltf_free(data);
    }

    UnloadFileData(fileData);

    return result == cgltf_result_success;
}
