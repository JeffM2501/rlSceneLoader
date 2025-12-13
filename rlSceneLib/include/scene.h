#pragma once

#include "raylib.h"
#include "raymath.h"

#include <memory>
#include <vector>
#include <unordered_map>
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
    BoundingBox Bounds = { 0 };

    struct MeshInstanceData
    {
        Material MaterialData;
        std::shared_ptr<Mesh> MeshData = nullptr;
    };

    std::vector<MeshInstanceData> Meshes;

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
    float Intensity = 1.0f;
    float Range = 100.0f;
    float MinCone = 0;
    float MaxCone = 0;

    enum class LightTypes
    {
        Point,
        Directional,
        Spot
    };

    LightTypes LightType = LightTypes::Point;

    LightSceneObject()
    {
        Type = SceneObjectType::LightObject;
    }
};

struct Scene
{
    std::unordered_map<size_t, Texture> TextureCache;
    std::unordered_map<size_t, std::shared_ptr<Mesh>> MeshCache;
    std::vector<std::unique_ptr<SceneObject>> RootObjects;

    std::vector<CameraSceneObject*> Cameras;
    std::vector<LightSceneObject*> Lights;
    std::vector<MeshSceneObject*> Meshes;
};

