#pragma once

#include "raylib.h"
#include "raymath.h"

#include <memory>
#include <vector>
#include <string>

enum class SceneObjectType
{
    GenericObject,
    MeshObject,
    LightObject,
    CameraObject
};

struct PQSTransform
{
    Vector3 position = Vector3Zeros;               // Position in 3D space
    Quaternion rotation = QuaternionIdentity();     // Rotation as a quaternion
    Vector3 scale = Vector3Ones;                    // Scale in 3D space

    PQSTransform()
        : position(Vector3Zeros), rotation(QuaternionIdentity()), scale(Vector3Ones) {
    }
    PQSTransform(const Vector3& pos, const Quaternion& rot, const Vector3& scl)
        : position(pos), rotation(rot), scale(scl) {
    }
};

struct SceneObject
{
protected:
    SceneObjectType Type = SceneObjectType::GenericObject;

public:
    SceneObjectType GetType() const { return Type; }

    std::string Name;
    PQSTransform Transform;

    Matrix WorldMatrix;

    SceneObject* Parent = nullptr;
    std::vector<std::unique_ptr<SceneObject>> Children;
    virtual ~SceneObject() = default;

    void CacheTransform();
};

struct MeshSceneObject : public SceneObject
{
    std::shared_ptr<Mesh> MeshData = nullptr;
    Material MaterialData = { 0 };

    MeshSceneObject()
    {
        Type = SceneObjectType::MeshObject;
    }
};

struct CameraSceneObject : public SceneObject
{
    float FOV = 45.0f;

    CameraSceneObject()
    {
        Type = SceneObjectType::CameraObject;
    }
};

struct LightSceneObject : public SceneObject
{
    Color EmissiveColor = WHITE;
    float Falloff = 100.0f;

    LightSceneObject()
    {
        Type = SceneObjectType::LightObject;
    }
};

struct Scene
{
    std::vector<std::shared_ptr<Mesh>> Models;
    std::vector<std::unique_ptr<SceneObject>> RootObjects;

    std::vector<CameraSceneObject*> Cameras;
    std::vector<LightSceneObject*> Lights;
    std::vector<MeshSceneObject*> Meshes;
};

