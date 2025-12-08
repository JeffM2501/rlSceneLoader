#pragma once

#include "scene.h"

#include <string_view>

bool LoadSceneFromGLTF(std::string_view filename, Scene& outScene);