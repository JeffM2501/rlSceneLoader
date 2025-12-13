#include "raylib.h"
#include "rlgl.h"
#include "raymath.h"

#include "scene.h"  
#include "scene_loader.h"

#define RLIGHTS_IMPLEMENTATION
#include "rlights.h"

Scene TestScene;

Camera3D ViewCamera = { 0 };
bool RegenerateTransforms = false;

Material DefaultMat = { 0 };

Shader LightShader = { 0 };

void GameInit()
{
    SetConfigFlags(FLAG_VSYNC_HINT | FLAG_WINDOW_RESIZABLE | FLAG_WINDOW_HIGHDPI | FLAG_MSAA_4X_HINT);
    InitWindow(1280, 800, "Example");
    SetTargetFPS(144);

    // load resources

    ViewCamera.fovy = 45.0f;
    ViewCamera.up = { 0, 1, 0 };
    ViewCamera.target = { 0, 0, 0 };
    ViewCamera.position = { 0, 5, -10 };

    LightShader = LoadShader("resources/lighting.vs", "resources/lighting.fs");

    LightShader.locs[SHADER_LOC_VECTOR_VIEW] = GetShaderLocation(LightShader, "viewPos");
	// NOTE: "matModel" location name is automatically assigned on shader loading,
	// no need to get the location again if using that uniform name
	//shader.locs[SHADER_LOC_MATRIX_MODEL] = GetShaderLocation(shader, "matModel");

	// Ambient light level (some basic lighting)
	int ambientLoc = GetShaderLocation(LightShader, "ambient");

    float ambient[4] = { 0.1f, 0.1f, 0.1f, 1.0f };
	SetShaderValue(LightShader, ambientLoc, ambient, SHADER_UNIFORM_VEC4);
    
    //LoadSceneFromGLTF("resources/normal.glb", TestScene);

 
    LoadSceneFromGLTF("resources/DungeonScene.glb", TestScene);

	for (auto* camera : TestScene.Cameras)
	{
		ViewCamera.fovy = camera->FOV;

		ViewCamera.position = Vector3Transform(Vector3Zeros, camera->WorldMatrix);
		ViewCamera.target = Vector3Transform(Vector3UnitZ, camera->WorldMatrix) - ViewCamera.position;
	}

    for (auto& meshNode : TestScene.Meshes)
    {
        for (auto& subMesh : meshNode->Meshes)
		{
            subMesh.MaterialData.shader = LightShader;
		}
    }

	int lightCount = 0;
	for (auto& lightNode : TestScene.Lights)
	{
		if (lightCount < 4)
		{
 			int lightType = LIGHT_POINT;
 			switch (lightNode->LightType)
 			{
 			case LightSceneObject::LightTypes::Directional:
 				lightType = LIGHT_DIRECTIONAL;
 				break;
 			case LightSceneObject::LightTypes::Spot:
 				lightType = LIGHT_POINT;
 				break;
 			case LightSceneObject::LightTypes::Point:
 				lightType = LIGHT_POINT;
 				break;
 			default:
 				lightType = LIGHT_POINT;
 				break;
 			}

			Vector3 lightPos = Vector3Transform(Vector3Zeros, lightNode->WorldMatrix);
			Vector3 lightTarget = Vector3Transform(Vector3UnitZ, lightNode->WorldMatrix);
		    CreateLight(lightType, lightPos, lightTarget, lightNode->EmissiveColor, LightShader);
		}

		lightCount++;
	}

    if (lightCount == 0)
    {
		CreateLight(LIGHT_DIRECTIONAL, Vector3{ -2, 1, -2 }, Vector3Zeros, WHITE, LightShader);
    }

    for (auto& [hash, mesh] : TestScene.MeshCache)
    {
        UploadMesh(mesh.get(), false);

        if (mesh->vertices)
        {
            MemFree(mesh->vertices);
            mesh->vertices = nullptr;
        }
        if (mesh->texcoords)
        {
            MemFree(mesh->texcoords);
            mesh->texcoords = nullptr;
        }
        if (mesh->normals)
        {
            MemFree(mesh->normals);
            mesh->normals = nullptr;
        }
        if (mesh->colors)
        {
            MemFree(mesh->colors);
            mesh->colors = nullptr;
        }
    }

    DefaultMat = LoadMaterialDefault();
}

void GameCleanup()
{
    // unload resources
    for (auto& [hash, mesh] : TestScene.MeshCache)
    {
        UnloadMesh(*mesh.get());
    }

    for (auto& [hash, texture] : TestScene.TextureCache)
    {
        UnloadTexture(texture);
    }
    CloseWindow();
}

bool GameUpdate()
{
    if (IsMouseButtonDown(MOUSE_BUTTON_RIGHT))
    {
        Vector3 movement = { 0 };
        if (IsKeyDown(KEY_W))
            movement.x += 1.0f;
        if (IsKeyDown(KEY_S))
            movement.x -= 1.0f;

        if (IsKeyDown(KEY_D))
            movement.y += 1.0f;
        if (IsKeyDown(KEY_A))
            movement.y -= 1.0f;

        if (IsKeyDown(KEY_Q))
            movement.z -= 1.0f;
        if (IsKeyDown(KEY_E))
            movement.z += 1.0f;

        float speed = 10;
        if (IsKeyDown(KEY_LEFT_SHIFT) || IsKeyDown(KEY_RIGHT_SHIFT))
            speed *= 5;

        movement *= GetFrameTime() * speed;

        Vector3 rotation = { 0 };
        rotation.x = GetMouseDelta().x;
        rotation.y = GetMouseDelta().y;

        rotation *= 0.1f;

        float zoom = 0;
        if (GetMouseWheelMove() > 0)
            zoom = 1;
        if (GetMouseWheelMove() < 0)
            zoom = -1;

        UpdateCameraPro(&ViewCamera, movement, rotation, zoom);
    }
	float cameraPos[3] = { ViewCamera.position.x, ViewCamera.position.y, ViewCamera.position.z };
	SetShaderValue(LightShader, LightShader.locs[SHADER_LOC_VECTOR_VIEW], cameraPos, SHADER_UNIFORM_VEC3);

    RegenerateTransforms = false;

    if (IsKeyPressed(KEY_F1))
        RegenerateTransforms = true;
    return true;
}

void DrawNode(SceneObject* node)
{
    if (RegenerateTransforms)
        node->CacheTransform();

    rlPushMatrix();
    rlMultMatrixf(MatrixToFloat(node->WorldMatrix));

    auto inverseScale = Vector3Invert(node->Transform.scale);

    DrawLine3D(Vector3{ inverseScale.x,0,0 }, Vector3{ -inverseScale.x,0,0 }, RED);
    DrawLine3D(Vector3{ 0,inverseScale.y,0 }, Vector3{ 0,-inverseScale.y,0 }, GREEN);
    DrawLine3D(Vector3{ 0,0,inverseScale.z }, Vector3{ 0,0,-inverseScale.z }, BLUE);

    switch (node->GetType())
    {
    case SceneObjectType::GenericObject:
        break;

    case SceneObjectType::MeshObject:
    {
        MeshSceneObject* mesh = dynamic_cast<MeshSceneObject*>(node);

//         for (auto& subMesh : mesh->Meshes)
//         {
//             DrawMesh(*subMesh.MeshData.get(), subMesh.MaterialData, MatrixIdentity());
//         }

        DrawBoundingBox(mesh->Bounds, GREEN);

        break;
    }

    case SceneObjectType::LightObject:
    {
        LightSceneObject* light = dynamic_cast<LightSceneObject*>(node);
        switch (light->LightType)
        {
        case LightSceneObject::LightTypes::Directional:
            //	rlRotatef(-45, 1, 0, 0);
            DrawCylinderWires(Vector3{ 0,-1.0f, 0 }, 0.1f, 2.0f, 0.5F, 10, light->EmissiveColor);
            break;

        case LightSceneObject::LightTypes::Spot:
            rlRotatef(90, 1, 0, 0);
            DrawCylinderWires(Vector3{ 0,-light->Range, 0 }, 0.125f, light->Range * tanf(light->MaxCone), light->Range, 12, light->EmissiveColor);
            break;

        case LightSceneObject::LightTypes::Point:
            rlRotatef(90, 1, 0, 0);
            DrawSphere(Vector3Zeros, 0.5f, light->EmissiveColor);
            DrawCylinder(Vector3{ 0,0.4f,0 }, 0.20f, 0.25f, 0.4f, 10, GRAY);

            DrawSphereWires(Vector3Zeros, light->Range, 8,8, ColorAlpha(light->EmissiveColor, 0.25f));
            break;

        default:
            break;
        }

        break;
    }

    case SceneObjectType::CameraObject:
    {
        rlRotatef(90, 1, 0, 0);
        DrawCylinderWires(Vector3{ 0,-0.5f,0 }, 0.25f, 0.5F, 0.5f, 10, BLACK);
        DrawCubeWires(Vector3{ 0,1.0f,0 }, 0.75f, 2, 1.0f, BLACK);
        break;
    }

    default:
        break;
    }

    rlPopMatrix();

    for (auto& child : node->Children)
    {
        DrawNode(child.get());
    }
}

void GameDraw()
{
    BeginDrawing();
    ClearBackground(DARKGRAY);

    BeginMode3D(ViewCamera);
    DrawGrid(200, 1.0f);

    DrawLine3D(Vector3{ 100,0.01f,0 }, Vector3{ -100, 0.01f, 0 }, RED);
    DrawLine3D(Vector3{ 0,0.01f,100 }, Vector3{ 0, 0.01f, -100 }, BLUE);

    // draw the meshes
    for (auto& meshNode : TestScene.Meshes)
    {
        for (auto& subMesh : meshNode->Meshes)
        {
            DrawMesh(*subMesh.MeshData.get(), subMesh.MaterialData, meshNode->WorldMatrix);
        }
    }
    rlDrawRenderBatchActive();
 //   rlDisableDepthTest();
    for (auto& node : TestScene.RootObjects)
        DrawNode(node.get());
    rlDrawRenderBatchActive();
   // rlEnableDepthTest();

    EndMode3D();

    DrawFPS(5, 0);
    DrawText(TextFormat("Unique Meshes %d", TestScene.MeshCache.size()), 5, 20, 20, BLACK);
    DrawText(TextFormat("Mesh Nodes %d", TestScene.Meshes.size()), 5, 40, 20, BLACK);
    EndDrawing();
}

int main()
{
    GameInit();

    while (!WindowShouldClose())
    {
        if (!GameUpdate())
            break;

        GameDraw();
    }
    GameCleanup();

    return 0;
}