#include "frustum_collision.h"
#include "rlgl.h"
#include <cmath>

void ViewCamera::SetPlanes(float near, float far)
{
    NearPlaneDistance = near;
    FarPlaneDistance = far;
    rlSetClipPlanes(near, far);
}

bool ExtractFrustumPlanes(ViewCamera& camera)
{
    float aspectRatio = (float)GetScreenWidth() / (float)GetScreenHeight();

    // Calculate camera orientation vectors
    Vector3 forward = Vector3Normalize(Vector3Subtract(camera.Camera.target, camera.Camera.position));
    Vector3 right = Vector3Normalize(Vector3CrossProduct(forward, camera.Camera.up));
    Vector3 up = Vector3CrossProduct(right, forward);

    // Calculate half heights and widths at near and far distances
    float nearHeight = tanf(camera.Camera.fovy * 0.5f * DEG2RAD) * camera.NearPlaneDistance;
    float nearWidth = nearHeight * aspectRatio;
    float farHeight = tanf(camera.Camera.fovy * 0.5f * DEG2RAD) * camera.FarPlaneDistance;
    float farWidth = farHeight * aspectRatio;

    // Near plane
    camera.Frustum.Near.Normal = forward;
    camera.Frustum.Near.Distance = Vector3DotProduct(forward, Vector3Add(camera.Camera.position, Vector3Scale(forward, camera.NearPlaneDistance)));

    // Far plane (normal points inward, toward camera)
    camera.Frustum.Far.Normal = Vector3Negate(forward);
    camera.Frustum.Far.Distance = Vector3DotProduct(Vector3Negate(forward), Vector3Add(camera.Camera.position, Vector3Scale(forward, camera.FarPlaneDistance)));

    // Calculate center points for near and far planes
    Vector3 nearCenter = Vector3Add(camera.Camera.position, Vector3Scale(forward, camera.NearPlaneDistance));
    Vector3 farCenter = Vector3Add(camera.Camera.position, Vector3Scale(forward, camera.FarPlaneDistance));

    // Left plane
    Vector3 leftNearPoint = Vector3Add(nearCenter, Vector3Scale(right, -nearWidth));
    Vector3 leftFarPoint = Vector3Add(farCenter, Vector3Scale(right, -farWidth));
    Vector3 leftEdge = Vector3Normalize(Vector3Subtract(leftFarPoint, camera.Camera.position));
    camera.Frustum.Left.Normal = Vector3Negate(Vector3Normalize(Vector3CrossProduct(up, leftEdge)));
    camera.Frustum.Left.Distance = Vector3DotProduct(camera.Frustum.Left.Normal, camera.Camera.position);

    // Right plane
    Vector3 rightNearPoint = Vector3Add(nearCenter, Vector3Scale(right, nearWidth));
    Vector3 rightFarPoint = Vector3Add(farCenter, Vector3Scale(right, farWidth));
    Vector3 rightEdge = Vector3Normalize(Vector3Subtract(rightFarPoint, camera.Camera.position));
    camera.Frustum.Right.Normal = Vector3Negate(Vector3Normalize(Vector3CrossProduct(rightEdge, up)));
    camera.Frustum.Right.Distance = Vector3DotProduct(camera.Frustum.Right.Normal, camera.Camera.position);
    // Top plane
    Vector3 topNearPoint = Vector3Add(nearCenter, Vector3Scale(up, nearHeight));
    Vector3 topFarPoint = Vector3Add(farCenter, Vector3Scale(up, farHeight));
    Vector3 topEdge = Vector3Normalize(Vector3Subtract(topFarPoint, camera.Camera.position));
    camera.Frustum.Top.Normal = Vector3Negate(Vector3Normalize(Vector3CrossProduct(right, topEdge)));
    camera.Frustum.Top.Distance = Vector3DotProduct(camera.Frustum.Top.Normal, camera.Camera.position);

    // Bottom plane
    Vector3 bottomNearPoint = Vector3Add(nearCenter, Vector3Scale(up, -nearHeight));
    Vector3 bottomFarPoint = Vector3Add(farCenter, Vector3Scale(up, -farHeight));
    Vector3 bottomEdge = Vector3Normalize(Vector3Subtract(bottomFarPoint, camera.Camera.position));
    camera.Frustum.Bottom.Normal = Vector3Negate(Vector3Normalize(Vector3CrossProduct(bottomEdge, right)));
    camera.Frustum.Bottom.Distance = Vector3DotProduct(camera.Frustum.Bottom.Normal, camera.Camera.position);

    return true;
}

float GetSignedDistanceToPlane(const Plane& plane, const Vector3& point)
{
    // Calculate signed distance: positive = in front of plane, negative = behind plane
    return Vector3DotProduct(plane.Normal, point) - plane.Distance;
}
bool IsSphereInFrustum(const CameraFrustum& frustum, const Vector3& center, float radius)
{
    // Check sphere against all 6 frustum planes
    // If the sphere is completely outside any plane, it's not in the frustum

    if (GetSignedDistanceToPlane(frustum.Near, center) < -radius)
        return false;

    if (GetSignedDistanceToPlane(frustum.Far, center) < -radius)
        return false;

    if (GetSignedDistanceToPlane(frustum.Left, center) < -radius)
        return false;

    if (GetSignedDistanceToPlane(frustum.Right, center) < -radius)
        return false;

    if (GetSignedDistanceToPlane(frustum.Top, center) < -radius)
        return false;

    if (GetSignedDistanceToPlane(frustum.Bottom, center) < -radius)
        return false;

    // Sphere is at least partially inside the frustum
    return true;
}

bool IsBoxInFrustum(const CameraFrustum& frustum, const BoundingBox& box, Matrix transform)
{
    // Get the 8 corners of the oriented box
   // Vector3 halfSize = Vector3Scale(box.Size, 0.5f);

    // Calculate local space corner offsets
    Vector3 corners[8] =
    {
        { box.min.x, box.min.y, box.min.z },
        { box.max.x, box.min.y, box.min.z },
        { box.min.x, box.max.y, box.min.z },
        { box.max.x, box.max.y, box.min.z },
        { box.min.x, box.min.y, box.max.z },
        { box.max.x, box.min.y, box.max.z },
        { box.min.x, box.max.y, box.max.z },
        { box.max.x, box.max.y, box.max.z }
    };

    for (int i = 0; i < 8; i++)
    {
        // Transform the corner to world space using the provided transform matrix
        corners[i] = Vector3Transform(corners[i], transform);
    }


    // Test the box against each frustum plane
    // For each plane, if all 8 corners are outside, the box is completely outside
    Plane planes[6] = { frustum.Near, frustum.Far, frustum.Left, frustum.Right, frustum.Top, frustum.Bottom };

    for (int p = 0; p < 6; p++)
    {
        int cornersOutside = 0;
        for (int c = 0; c < 8; c++)
        {
            if (GetSignedDistanceToPlane(planes[p], corners[c]) < 0)
            {
                cornersOutside++;
            }
        }

        // If all 8 corners are outside this plane, the box is not in the frustum
        if (cornersOutside == 8)
        {
            return false;
        }
    }

    // Box is at least partially inside the frustum
    return true;
}