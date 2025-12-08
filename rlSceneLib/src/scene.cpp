
#include "scene.h"

void SceneObject::CacheTransform()
{
    WorldMatrix = MatrixMultiply(MatrixScale(Transform.scale.x, Transform.scale.y, Transform.scale.z),
        MatrixMultiply(QuaternionToMatrix(Transform.rotation),
            MatrixTranslate(Transform.position.x, Transform.position.y, Transform.position.z)));

    if (Parent)
    {
        WorldMatrix = MatrixMultiply(Parent->WorldMatrix, WorldMatrix);
    }
}
