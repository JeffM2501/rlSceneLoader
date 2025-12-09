#include "raylib.h"
#include "rlgl.h"
#include "raymath.h"

#include "scene.h"  
#include "scene_loader.h"

Scene TestScene;

Camera3D ViewCamera = { 0 };
bool RegenerateTransforms = false;

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

  //  LoadSceneFromGLTF("resources/normal.glb", TestScene);
    LoadSceneFromGLTF("resources/optimzed_scene.glb", TestScene);

    for (auto* camera : TestScene.Cameras)
    {
		ViewCamera.fovy = camera->FOV;
	
        ViewCamera.position = Vector3Transform(Vector3Zeros, camera->WorldMatrix);
		ViewCamera.target = Vector3Transform(Vector3UnitZ, camera->WorldMatrix) - ViewCamera.position;
    }
}

void GameCleanup()
{
    // unload resources

    CloseWindow();
}

bool GameUpdate()
{
    if (IsMouseButtonDown(MOUSE_BUTTON_RIGHT))
        UpdateCamera(&ViewCamera, CAMERA_FIRST_PERSON);

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
	    break;

    case SceneObjectType::LightObject:
    {
		LightSceneObject* light = dynamic_cast<LightSceneObject*>(node);
        DrawSphereWires(Vector3Zeros, light->Intensity, 8, 8, light->EmissiveColor);
        break;
	}

	case SceneObjectType::CameraObject:
	{
        rlRotatef(90, 1, 0, 0);
		DrawCylinderWires(Vector3{0,-0.5f,0}, 0.25f,0.5F, 0.5f, 10, BLACK);
        DrawCubeWires(Vector3{ 0,1.0f,0 }, 0.75f, 2, 1.5f, BLACK);
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

    for (auto& node : TestScene.RootObjects)
        DrawNode(node.get());

    EndMode3D();

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