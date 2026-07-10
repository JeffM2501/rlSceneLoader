#pragma once

#include "scene.h"
#include "frustum_collision.h"
#include <vector>
#include <functional>

using ObjectPtrList = std::vector<SceneObject*>;

class SceneGraph
{
public:
    Scene& SceneData;
    virtual ~SceneGraph() = default;

    SceneGraph(Scene& scene) : SceneData(scene) {}
    virtual size_t Query(const Vector3& center, float radius, ObjectPtrList& objectList) = 0;
    virtual size_t Query(const BoundingBox& boundingBox, ObjectPtrList& objectList) = 0;
    virtual size_t Query(ViewCamera& camera, ObjectPtrList& objectList) = 0;
};

class BruteForceSceneGraph : public SceneGraph
{
private:
    void DoForEachObject(SceneObject* node, const std::function<void(SceneObject*)>& func);
public:
    BruteForceSceneGraph(Scene& scene);

    size_t Query(const Vector3& center, float radius, ObjectPtrList& objectList) override;
    size_t Query(const BoundingBox& boundingBox, ObjectPtrList& objectList) override;
    size_t Query(ViewCamera& camera, ObjectPtrList& objectList) override;
};
