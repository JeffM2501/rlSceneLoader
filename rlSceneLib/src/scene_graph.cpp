#include "scene_graph.h"

#include "raylib.h"

#include <map>
#include <cmath>
#include "rlgl.h"


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

size_t BruteForceSceneGraph::Query(ViewCamera& camera, ObjectPtrList& objectList)
{
    ExtractFrustumPlanes(const_cast<ViewCamera&>(camera));

    size_t count = objectList.size();
    DoForEachObject(nullptr, [&](SceneObject* obj) {
        if (obj->IsRenderable())
        {
            const BoundingBox* bbox = obj->GetBoundingBox();
            if (bbox && IsBoxInFrustum(camera.Frustum, *bbox, obj->WorldMatrix))
            {
                objectList.push_back(obj);
            }
        }
    });
    return objectList.size() - count;
}
