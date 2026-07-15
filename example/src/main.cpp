#include "raylib.h"
#include "rlgl.h"
#include "raymath.h"

#include "scene.h"  
#include "scene_loader.h"

#include "scene_graph.h"
#include "frustum_collision.h"

#define RLIGHTS_IMPLEMENTATION
#include "rlights.h"

Scene TestScene;
BruteForceSceneGraph Graph(TestScene);

ViewCamera Cam;
ViewCamera GameCam;
bool RegenerateTransforms = false;

bool UseGameCam = true;


bool ShowWires = false;

Material DefaultMat = { 0 };

Shader LightShader = { 0 };

ViewCamera& GetActiveCamera()
{
    return UseGameCam ? GameCam : Cam;
}

ViewCamera& GetAlternateCamera()
{
    return !UseGameCam ? GameCam : Cam;
}

void GameInit()
{
    SetConfigFlags(FLAG_VSYNC_HINT | FLAG_WINDOW_RESIZABLE | FLAG_WINDOW_HIGHDPI | FLAG_MSAA_4X_HINT);
    InitWindow(1280, 800, "Example");
    SetTargetFPS(144);

    // load resources

    Cam.Camera.fovy = 45.0f;
    Cam.Camera.up = { 0, 1, 0 };
    Cam.Camera.target = { 0, 0, 0 };
    Cam.Camera.position = { 0, 5, -10 };

    GameCam.Camera = Cam.Camera;

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
        GameCam.Camera.fovy = camera->FOV;

        GameCam.Camera.position = Vector3Transform(Vector3Zeros, camera->WorldMatrix);
        GameCam.Camera.target = Vector3Transform(Vector3UnitZ, camera->WorldMatrix) - GameCam.Camera.position;
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
		    CreateLight(lightType, lightPos, lightTarget, lightNode->EmissiveColor, LightShader, lightNode->Intensity);
		}

		lightCount++;
	}

    if (lightCount == 0)
    {
		CreateLight(LIGHT_DIRECTIONAL, Vector3{ -2, 1, -2 }, Vector3Zeros, WHITE, LightShader, 1.0f);
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
    if (IsKeyPressed(KEY_TAB))
        UseGameCam = !UseGameCam;

    if (IsKeyPressed(KEY_F1))
        ShowWires = !ShowWires;

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
 
    if (IsMouseButtonDown(MOUSE_BUTTON_RIGHT))
        UpdateCameraPro(&GetActiveCamera().Camera, movement, rotation, zoom);
    if (IsMouseButtonDown(MOUSE_BUTTON_LEFT) && IsKeyDown(KEY_LEFT_CONTROL))
        UpdateCameraPro(&GetAlternateCamera().Camera, movement, rotation, zoom);

	float cameraPos[3] = { GetActiveCamera().Camera.position.x, GetActiveCamera().Camera.position.y, GetActiveCamera().Camera.position.z};
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
    if (ShowWires)
    {
        DrawLine3D(Vector3{ inverseScale.x,0,0 }, Vector3{ -inverseScale.x,0,0 }, RED);
        DrawLine3D(Vector3{ 0,inverseScale.y,0 }, Vector3{ 0,-inverseScale.y,0 }, GREEN);
        DrawLine3D(Vector3{ 0,0,inverseScale.z }, Vector3{ 0,0,-inverseScale.z }, BLUE);
    }

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

        if (ShowWires)
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
            if (ShowWires)
                DrawCylinderWires(Vector3{ 0,-1.0f, 0 }, 0.1f, 2.0f, 0.5F, 10, light->EmissiveColor);
            break;

        case LightSceneObject::LightTypes::Spot:
            rlRotatef(90, 1, 0, 0);
            if (ShowWires)
                DrawCylinderWires(Vector3{ 0,-light->Range, 0 }, 0.125f, light->Range * tanf(light->MaxCone), light->Range, 12, light->EmissiveColor);
            break;

        case LightSceneObject::LightTypes::Point:
            rlRotatef(90, 1, 0, 0);
            
            if (ShowWires)
            {
                DrawSphere(Vector3Zeros, 0.5f, light->EmissiveColor);
                DrawCylinder(Vector3{ 0,0.4f,0 }, 0.20f, 0.25f, 0.4f, 10, GRAY);

                DrawSphereWires(Vector3Zeros, light->Range, 8, 8, ColorAlpha(light->EmissiveColor, 0.25f));
            }
            break;

        default:
            break;
        }

        break;
    }

    case SceneObjectType::CameraObject:
    {
        rlRotatef(90, 1, 0, 0);
        if (ShowWires)
        {
            DrawCylinderWires(Vector3{ 0,0.0f, 0 }, 0.25f, 0.5F, 0.5f, 10, BLACK);
            DrawCubeWires(Vector3{ 0,1.5f, 0 }, 0.75f, 2, 1.0f, BLACK);
        }
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

Quaternion QuaternionFromCamera(Camera3D camera)
{
    // Calculate camera's orientation vectors
    Vector3 forward = Vector3Normalize(Vector3Subtract(camera.target, camera.position));
    Vector3 right = Vector3Normalize(Vector3CrossProduct(forward, camera.up));
    Vector3 up = Vector3CrossProduct(right, forward);

    // Build a rotation matrix from the camera's orientation
    // Matrix layout in raylib is column-major
    Matrix rotationMatrix =
    {
        right.x, up.x, -forward.x, 0.0f,
        right.y, up.y, -forward.y, 0.0f,
        right.z, up.z, -forward.z, 0.0f,
        0.0f, 0.0f, 0.0f, 1.0f
    };

    // Convert the rotation matrix to a quaternion
    return QuaternionFromMatrix(rotationMatrix);
}

void VisualizeCamera(ViewCamera& camera)
{
    auto cameraQuat = QuaternionFromCamera(camera.Camera);

    rlPushMatrix();
    rlTranslatef(camera.Camera.position.x, camera.Camera.position.y, camera.Camera.position.z);
    Vector3 axis = Vector3Zeros;
    float angle = 0;

    QuaternionToAxisAngle(cameraQuat, &axis, &angle);

    rlRotatef(angle * RAD2DEG, axis.x, axis.y, axis.z);

    rlRotatef(90, 1, 0, 0);
    BeginShaderMode(LightShader);
    DrawCylinder(Vector3{ 0,0.0f, 0 }, 0.25f, 0.5F, 0.5f, 10, DARKBLUE);
    DrawCube(Vector3{ 0,1.5f, 0 }, 0.75f, 2, 1.0f, DARKBLUE);
    EndShaderMode();
    rlPopMatrix();

    // Draw a line from the camera position to the target
    DrawLine3D(camera.Camera.position, camera.Camera.target, GREEN);
    DrawLine3D(camera.Camera.position, camera.Camera.position + (camera.Camera.up * 2), PURPLE);

    // Calculate frustum parameters
    float nearPlane = camera.NearPlaneDistance * 100;
    float farPlane = camera.FarPlaneDistance * 0.1f;
    float aspectRatio = (float)GetScreenWidth() / (float)GetScreenHeight();

    // Calculate near and far plane dimensions
    float nearHeight = 2.0f * tanf(camera.Camera.fovy * 0.5f * DEG2RAD) * nearPlane;
    float nearWidth = nearHeight * aspectRatio;
    float farHeight = 2.0f * tanf(camera.Camera.fovy * 0.5f * DEG2RAD) * farPlane;
    float farWidth = farHeight * aspectRatio;

    // Calculate camera's forward and right vectors
    Vector3 forward = Vector3Normalize(Vector3Subtract(camera.Camera.target, camera.Camera.position));
    Vector3 right = Vector3Normalize(Vector3CrossProduct(forward, camera.Camera.up));
    Vector3 up = Vector3CrossProduct(right, forward);

    // Calculate near plane center and corners
    Vector3 nearCenter = Vector3Add(camera.Camera.position, Vector3Scale(forward, nearPlane));
    Vector3 nearTopLeft = Vector3Add(Vector3Add(nearCenter, Vector3Scale(up, nearHeight * 0.5f)), Vector3Scale(right, -nearWidth * 0.5f));
    Vector3 nearTopRight = Vector3Add(Vector3Add(nearCenter, Vector3Scale(up, nearHeight * 0.5f)), Vector3Scale(right, nearWidth * 0.5f));
    Vector3 nearBottomLeft = Vector3Add(Vector3Add(nearCenter, Vector3Scale(up, -nearHeight * 0.5f)), Vector3Scale(right, -nearWidth * 0.5f));
    Vector3 nearBottomRight = Vector3Add(Vector3Add(nearCenter, Vector3Scale(up, -nearHeight * 0.5f)), Vector3Scale(right, nearWidth * 0.5f));

    // Calculate far plane center and corners
    Vector3 farCenter = Vector3Add(camera.Camera.position, Vector3Scale(forward, farPlane));
    Vector3 farTopLeft = Vector3Add(Vector3Add(farCenter, Vector3Scale(up, farHeight * 0.5f)), Vector3Scale(right, -farWidth * 0.5f));
    Vector3 farTopRight = Vector3Add(Vector3Add(farCenter, Vector3Scale(up, farHeight * 0.5f)), Vector3Scale(right, farWidth * 0.5f));
    Vector3 farBottomLeft = Vector3Add(Vector3Add(farCenter, Vector3Scale(up, -farHeight * 0.5f)), Vector3Scale(right, -farWidth * 0.5f));
    Vector3 farBottomRight = Vector3Add(Vector3Add(farCenter, Vector3Scale(up, -farHeight * 0.5f)), Vector3Scale(right, farWidth * 0.5f));

    // Draw near plane (rectangle)
    DrawLine3D(nearTopLeft, nearTopRight, YELLOW);
    DrawLine3D(nearTopRight, nearBottomRight, YELLOW);
    DrawLine3D(nearBottomRight, nearBottomLeft, YELLOW);
    DrawLine3D(nearBottomLeft, nearTopLeft, YELLOW);

    // Draw far plane (rectangle)
    DrawLine3D(farTopLeft, farTopRight, ORANGE);
    DrawLine3D(farTopRight, farBottomRight, ORANGE);
    DrawLine3D(farBottomRight, farBottomLeft, ORANGE);
    DrawLine3D(farBottomLeft, farTopLeft, ORANGE);

    // Draw connecting lines (edges of frustum pyramid)
    DrawLine3D(camera.Camera.position, farTopLeft, SKYBLUE);
    DrawLine3D(camera.Camera.position, farTopRight, SKYBLUE);
    DrawLine3D(camera.Camera.position, farBottomLeft, SKYBLUE);
    DrawLine3D(camera.Camera.position, farBottomRight, SKYBLUE);
}

void GameDraw()
{
    BeginDrawing();
    ClearBackground(DARKGRAY);

    BeginMode3D(GetActiveCamera().Camera);
    DrawGrid(200, 1.0f);

    DrawLine3D(Vector3{ 100,0.01f,0 }, Vector3{ -100, 0.01f, 0 }, RED);
    DrawLine3D(Vector3{ 0,0.01f,100 }, Vector3{ 0, 0.01f, -100 }, BLUE);

    std::vector<SceneObject*> renderableObjects;
    Graph.Query(GameCam,  renderableObjects);
    //Graph.Query(ViewCamera.position, 20, renderableObjects);
    // draw the meshes
    for (auto& node : renderableObjects)
    {
        MeshSceneObject* meshNode = dynamic_cast<MeshSceneObject*>(node);

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

    VisualizeCamera(GameCam);

    EndMode3D();

    DrawFPS(5, 0);
    DrawText(TextFormat("Unique Meshes %d", TestScene.MeshCache.size()), 5, 20, 20, BLACK);
    DrawText(TextFormat("Mesh Nodes %d", TestScene.Meshes.size()), 5, 40, 20, BLACK);
    DrawText(TextFormat("Drawn Nodes %d", renderableObjects.size()), 5, 60, 20, BLACK);
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