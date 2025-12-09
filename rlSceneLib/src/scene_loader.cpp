#include "scene_loader.h"

#include "raylib.h"
#include "external/cgltf.h"

#include <unordered_map>

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
			light->MinCone = node->light->spot_inner_cone_angle;
			light->MaxCone = node->light->spot_outer_cone_angle;
            break;
        }

        outScene.Lights.push_back(light);
	}
	else if (node->mesh)
	{
        sceneNode = std::make_unique<MeshSceneObject>();
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
