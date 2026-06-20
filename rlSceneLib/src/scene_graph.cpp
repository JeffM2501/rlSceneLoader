#include "scene_graph.h"

#include "raylib.h"

#include <map>
#include <cmath>
#include "rlgl.h"

class RLFrustum
{
public:
    enum class FrustumPlanes
    {
        Back = 0,
        Front = 1,
        Bottom = 2,
        Top = 3,
        Right = 4,
        Left = 5,
        MAX = 6
    };

    std::map<FrustumPlanes, Vector4> Planes;

    RLFrustum();
    virtual ~RLFrustum() {}

    void Extract();

    bool PointIn(const Vector3& position) const;
    bool PointIn(float x, float y, float z) const;

    bool SphereIn(const Vector3& position, float radius) const;
    bool AABBoxIn(const Vector3& min, const Vector3& max) const;
};

void NormalizePlane(Vector4& plane)
{
    float magnitude = sqrtf(plane.x * plane.x + plane.y * plane.y + plane.z * plane.z);

    plane.x /= magnitude;
    plane.y /= magnitude;
    plane.z /= magnitude;
    plane.w /= magnitude;
}

RLFrustum::RLFrustum()
{
    Planes[FrustumPlanes::Right] = Vector4{ 0 };
    Planes[FrustumPlanes::Left] = Vector4{ 0 };
    Planes[FrustumPlanes::Top] = Vector4{ 0 };
    Planes[FrustumPlanes::Bottom] = Vector4{ 0 };
    Planes[FrustumPlanes::Front] = Vector4{ 0 };
    Planes[FrustumPlanes::Back] = Vector4{ 0 };
}

void RLFrustum::Extract()
{
    // Get the combined projection * modelview matrix
    Matrix projection = rlGetMatrixProjection();
    Matrix modelview = rlGetMatrixModelview();
    
    // Combine matrices: combined = projection * modelview
    Matrix combined = MatrixMultiply(modelview, projection);
    
    // Extract frustum planes from the combined matrix
    // Each row of the combined matrix represents a plane in homogeneous coordinates
    
    // Right plane: [m3 - m0, m7 - m4, m11 - m8, m15 - m12]
    Planes[FrustumPlanes::Right] = {
        combined.m3 - combined.m0,
        combined.m7 - combined.m4,
        combined.m11 - combined.m8,
        combined.m15 - combined.m12
    };
    NormalizePlane(Planes[FrustumPlanes::Right]);
    
    // Left plane: [m3 + m0, m7 + m4, m11 + m8, m15 + m12]
    Planes[FrustumPlanes::Left] = {
        combined.m3 + combined.m0,
        combined.m7 + combined.m4,
        combined.m11 + combined.m8,
        combined.m15 + combined.m12
    };
    NormalizePlane(Planes[FrustumPlanes::Left]);
    
    // Top plane: [m3 - m1, m7 - m5, m11 - m9, m15 - m13]
    Planes[FrustumPlanes::Top] = {
        combined.m3 - combined.m1,
        combined.m7 - combined.m5,
        combined.m11 - combined.m9,
        combined.m15 - combined.m13
    };
    NormalizePlane(Planes[FrustumPlanes::Top]);
    
    // Bottom plane: [m3 + m1, m7 + m5, m11 + m9, m15 + m13]
    Planes[FrustumPlanes::Bottom] = {
        combined.m3 + combined.m1,
        combined.m7 + combined.m5,
        combined.m11 + combined.m9,
        combined.m15 + combined.m13
    };
    NormalizePlane(Planes[FrustumPlanes::Bottom]);
    
    // Back plane (far): [m3 - m2, m7 - m6, m11 - m10, m15 - m14]
    Planes[FrustumPlanes::Back] = {
        combined.m3 - combined.m2,
        combined.m7 - combined.m6,
        combined.m11 - combined.m10,
        combined.m15 - combined.m14
    };
    NormalizePlane(Planes[FrustumPlanes::Back]);
    
    // Front plane (near): [m3 + m2, m7 + m6, m11 + m10, m15 + m14]
    Planes[FrustumPlanes::Front] = {
        combined.m3 + combined.m2,
        combined.m7 + combined.m6,
        combined.m11 + combined.m10,
        combined.m15 + combined.m14
    };
    NormalizePlane(Planes[FrustumPlanes::Front]);
}

float DistanceToPlane(const Vector4& plane, const Vector3& position)
{
    return (plane.x * position.x + plane.y * position.y + plane.z * position.z + plane.w);
}

float DistanceToPlane(const Vector4& plane, float x, float y, float z)
{
    return (plane.x * x + plane.y * y + plane.z * z + plane.w);
}

bool RLFrustum::PointIn(const Vector3& position) const
{
    for (auto& plane : Planes)
    {
        if (DistanceToPlane(plane.second, position) <= 0) // point is behind plane
            return false;
    }

    return true;
}

bool RLFrustum::PointIn(float x, float y, float z) const
{
    for (auto& plane : Planes)
    {
        if (DistanceToPlane(plane.second, x, y, z) <= 0) // point is behind plane
            return false;
    }

    return true;
}

bool RLFrustum::SphereIn(const Vector3& position, float radius) const
{
    for (auto& plane : Planes)
    {
        if (DistanceToPlane(plane.second, position) < -radius) // center is behind plane by more than the radius
            return false;
    }

    return true;
}

bool RLFrustum::AABBoxIn(const Vector3& min, const Vector3& max) const
{
    // if any point is in and we are good
    if (PointIn(min.x, min.y, min.z))
        return true;

    if (PointIn(min.x, max.y, min.z))
        return true;

    if (PointIn(max.x, max.y, min.z))
        return true;

    if (PointIn(max.x, min.y, min.z))
        return true;

    if (PointIn(min.x, min.y, max.z))
        return true;

    if (PointIn(min.x, max.y, max.z))
        return true;

    if (PointIn(max.x, max.y, max.z))
        return true;

    if (PointIn(max.x, min.y, max.z))
        return true;

    // check to see if all points are outside of any one plane, if so the entire box is outside
    for (auto& plane : Planes)
    {
        bool oneInside = false;

        if (DistanceToPlane(plane.second, min.x, min.y, min.z) >= 0)
            oneInside = true;

        if (DistanceToPlane(plane.second, max.x, min.y, min.z) >= 0)
            oneInside = true;

        if (DistanceToPlane(plane.second, max.x, max.y, min.z) >= 0)
            oneInside = true;

        if (DistanceToPlane(plane.second, min.x, max.y, min.z) >= 0)
            oneInside = true;

        if (DistanceToPlane(plane.second, min.x, min.y, max.z) >= 0)
            oneInside = true;

        if (DistanceToPlane(plane.second, max.x, min.y, max.z) >= 0)
            oneInside = true;

        if (DistanceToPlane(plane.second, max.x, max.y, max.z) >= 0)
            oneInside = true;

        if (DistanceToPlane(plane.second, min.x, max.y, max.z) >= 0)
            oneInside = true;

        if (!oneInside)
            return false;
    }

    // the box extends outside the frustum but crosses it
    return true;
}

BruteForceSceneGraph::BruteForceSceneGraph(Scene& scene)
    : SceneGraph(scene)
{
}

void BruteForceSceneGraph::DoForEachObject(SceneObject* node, const std::function<void(SceneObject*)>& func)
{
    if (node == nullptr)
    {
        for (auto& rootNode : SceneData.RootObjects)
        {
            DoForEachObject(rootNode.get(), func);
        }
    }
    else
    {
        func(node);
        for (auto& child : node->Children)
        {
            DoForEachObject(child.get(), func);
        }
    }
}

size_t BruteForceSceneGraph::Query(const Vector3& center, float radius, ObjectPtrList& objectList)
{
    size_t count = objectList.size();
    DoForEachObject(nullptr, [&](SceneObject* obj) {
        if (obj->IsRenderable())
        {
            const BoundingBox* bbox = obj->GetBoundingBox();
            if (bbox && CheckCollisionBoxSphere(*bbox, center, radius))
            {
                objectList.push_back(obj);
            }
        }
    });
    return objectList.size() - count;
}

size_t BruteForceSceneGraph::Query(const BoundingBox& boundingBox, ObjectPtrList& objectList)
{
    size_t count = objectList.size();
    DoForEachObject(nullptr, [&](SceneObject* obj) {
        if (obj->IsRenderable())
        {
            const BoundingBox* bbox = obj->GetBoundingBox();
            if (bbox && CheckCollisionBoxes(*bbox, boundingBox))
            {
                objectList.push_back(obj);
            }
        }
        });
    return objectList.size() - count;
}

size_t BruteForceSceneGraph::Query(const Camera3D& camera, ObjectPtrList& objectList)
{
    RLFrustum frustum;
    frustum.Extract();

    size_t count = objectList.size();
    DoForEachObject(nullptr, [&](SceneObject* obj) {
        if (obj->IsRenderable())
        {
            const BoundingBox* bbox = obj->GetBoundingBox();
            if (bbox && frustum.AABBoxIn(bbox->min, bbox->max))
            {
                objectList.push_back(obj);
            }
        }
    });
    return objectList.size() - count;
}
