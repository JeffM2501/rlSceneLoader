/*
Raylib example file.
This is an example main file for a simple raylib project.
Use this as a starting point or replace it with your code.

-- Copyright (c) 2020-2024 Jeffery Myers
--
--This software is provided "as-is", without any express or implied warranty. In no event
--will the authors be held liable for any damages arising from the use of this software.

--Permission is granted to anyone to use this software for any purpose, including commercial
--applications, and to alter it and redistribute it freely, subject to the following restrictions:

--  1. The origin of this software must not be misrepresented; you must not claim that you
--  wrote the original software. If you use this software in a product, an acknowledgment
--  in the product documentation would be appreciated but is not required.
--
--  2. Altered source versions must be plainly marked as such, and must not be misrepresented
--  as being the original software.
--
--  3. This notice may not be removed or altered from any source distribution.

*/

#include "raylib.h"
#include "rlgl.h"
#include "raymath.h"

#include "scene.h"  
#include "scene_loader.h"

Scene TestScene;

Camera3D ViewCamera = { 0 };


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

    LoadSceneFromGLTF("resources/optimzed.glb", TestScene);
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
    return true;
}

void DrawNode(SceneObject* node)
{
    rlPushMatrix();
    rlMultMatrixf(MatrixToFloat(node->WorldMatrix));

    DrawCube(Vector3Zeros, 1, 1, 1, PURPLE);

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
    DrawGrid(100, 1.0f);

    DrawCube(Vector3Zeros, 1, 1, 1, RED);
    DrawSphere(Vector3{ 0,2,0 }, 0.5f, BLUE);

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