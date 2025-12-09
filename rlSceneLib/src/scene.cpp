
#include "scene.h"


void PQSTransformToMatrix(const PQSTransform& transform, Matrix& out_matrix)
{
	float lm[16] = { 0 };

	{
		float tx = transform.position.x;
		float ty = transform.position.y;
		float tz = transform.position.z;

		float qx = transform.rotation.x;
		float qy = transform.rotation.y;
		float qz = transform.rotation.z;
		float qw = transform.rotation.w;

		float sx = transform.scale.x;
		float sy = transform.scale.y;
		float sz = transform.scale.z;

		lm[0] = (1 - 2 * qy * qy - 2 * qz * qz) * sx;
		lm[1] = (2 * qx * qy + 2 * qz * qw) * sx;
		lm[2] = (2 * qx * qz - 2 * qy * qw) * sx;
		lm[3] = 0.f;

		lm[4] = (2 * qx * qy - 2 * qz * qw) * sy;
		lm[5] = (1 - 2 * qx * qx - 2 * qz * qz) * sy;
		lm[6] = (2 * qy * qz + 2 * qx * qw) * sy;
		lm[7] = 0.f;

		lm[8] = (2 * qx * qz + 2 * qy * qw) * sz;
		lm[9] = (2 * qy * qz - 2 * qx * qw) * sz;
		lm[10] = (1 - 2 * qx * qx - 2 * qy * qy) * sz;
		lm[11] = 0.f;

		lm[12] = tx;
		lm[13] = ty;
		lm[14] = tz;
		lm[15] = 1.f;
	}

	out_matrix = Matrix{
				lm[0], lm[4], lm[8], lm[12],
				lm[1], lm[5], lm[9], lm[13],
				lm[2], lm[6], lm[10], lm[14],
				lm[3], lm[7], lm[11], lm[15]
	};
}

void SceneObject::CacheTransform()
{
    PQSTransformToMatrix(Transform, WorldMatrix);

    if (Parent)
    {
        WorldMatrix = MatrixMultiply(WorldMatrix, Parent->WorldMatrix);
    }
}
