#pragma once

#include "scene.h"

#include <string_view>
#include <functional>

using ResolveTextureCallback = std::function < Image(std::string_view scenePath, std::string_view imageURL, size_t& hash)>;


void SetTextureResolver(ResolveTextureCallback resolver);

bool LoadSceneFromGLTF(std::string_view filename, Scene& outScene);