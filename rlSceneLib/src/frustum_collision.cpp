#include "frustum_collision.h"
#include <cmath>

// Helper function to get a point on the bounding box that is furthest in a given direction
Vector3 GetBoxFurthestPoint(const BoundingBox& box, const Vector3& direction)
{
    Vector3 result = box.min;

    if (direction.x > 0)
        result.x = box.max.x;

    if (direction.y > 0)
        result.y = box.max.y;

    if (direction.z > 0)
        result.z = box.max.z;

    return result;
}

// Helper function to get a point on the bounding box that is closest in a given direction
Vector3 GetBoxClosestPoint(const BoundingBox& box, const Vector3& direction)
{
    Vector3 result = box.max;

    if (direction.x > 0) 
        result.x = box.min.x;
    else if (direction.x < 0) 
        result.x = box.max.x;

    if (direction.y > 0) 
        result.y = box.min.y;
    else if (direction.y < 0) 
        result.y = box.max.y;

    if (direction.z > 0) 
        result.z = box.min.z;
    else if (direction.z < 0) 
        result.z = box.max.z;

    return result;
}

// Extract frustum planes from camera
void GetFrustumPlanes(const Camera3D& camera, Plane planes[6])
{
    // Get camera vectors
    Vector3 forward = Vector3Normalize(Vector3Subtract(camera.target, camera.position));
    Vector3 right = Vector3Normalize(Vector3CrossProduct(forward, camera.up));
    Vector3 up = Vector3Normalize(Vector3CrossProduct(right, forward));

    float fov_rad = camera.fovy * PI / 180.0f * 0.5f;
    float tan_fov = std::tanf(fov_rad);

    // Calculate aspect ratio (assuming standard 16:9, adjust as needed)
    float aspect = float(GetScreenWidth()) / float(GetScreenHeight());

    // Near and far planes
    float near_dist = 0.1f;
    float far_dist = 1000.0f;

    Vector3 center = Vector3Add(camera.position, Vector3Scale(forward, near_dist));

    // Near plane
    planes[4].Normal = Vector3Negate(forward);
    planes[4].Distance = Vector3DotProduct(planes[4].Normal, center);

    // Far plane
    center = Vector3Add(camera.position, Vector3Scale(forward, far_dist));
    planes[5].Normal = forward;
    planes[5].Distance = Vector3DotProduct(planes[5].Normal, center);

    // Calculate frustum height and width at near plane
    float near_height = 2.0f * near_dist * tan_fov;
    float near_width = near_height * aspect;

    // Left plane
    Vector3 left_vec = Vector3Scale(right, -near_width * 0.5f);
    Vector3 near_center = Vector3Add(camera.position, Vector3Scale(forward, near_dist));
    Vector3 near_left = Vector3Add(near_center, left_vec);
    Vector3 left_normal = Vector3CrossProduct(up, Vector3Subtract(near_left, camera.position));
    planes[0].Normal = Vector3Normalize(left_normal);
    planes[0].Distance = Vector3DotProduct(planes[0].Normal, camera.position);

    // Right plane
    Vector3 right_vec = Vector3Scale(right, near_width * 0.5f);
    Vector3 near_right = Vector3Add(near_center, right_vec);
    Vector3 right_normal = Vector3CrossProduct(Vector3Subtract(near_right, camera.position), up);
    planes[1].Normal = Vector3Normalize(right_normal);
    planes[1].Distance = Vector3DotProduct(planes[1].Normal, camera.position);

    // Top plane
    Vector3 top_vec = Vector3Scale(up, near_height * 0.5f);
    Vector3 near_top = Vector3Add(near_center, top_vec);
    Vector3 top_normal = Vector3CrossProduct(right, Vector3Subtract(near_top, camera.position));
    planes[2].Normal = Vector3Normalize(top_normal);
    planes[2].Distance = Vector3DotProduct(planes[2].Normal, camera.position);

    // Bottom plane
    Vector3 bottom_vec = Vector3Scale(up, -near_height * 0.5f);
    Vector3 near_bottom = Vector3Add(near_center, bottom_vec);
    Vector3 bottom_normal = Vector3CrossProduct(Vector3Subtract(near_bottom, camera.position), right);
    planes[3].Normal = Vector3Normalize(bottom_normal);
    planes[3].Distance = Vector3DotProduct(planes[3].Normal, camera.position);
}

// Check if a bounding box is inside or intersects the camera frustum
int CheckBoundingBoxFrustum(const BoundingBox& box, const Camera3D& camera)
{
    Plane planes[6];
    GetFrustumPlanes(camera, planes);

    bool allInside = true;

    // Test all 6 planes
    for (int i = 0; i < 6; i++)
    {
        // Get the furthest point from the plane normal
        Vector3 furthestPoint = GetBoxFurthestPoint(box, planes[i].Normal);
        float furthestDist = Vector3DotProduct(furthestPoint, planes[i].Normal) - planes[i].Distance;

        // If furthest point is behind the plane, box is completely outside
        if (furthestDist < 0)
            return -1; // Completely outside

        // Get the closest point to the plane normal
        Vector3 closestPoint = GetBoxClosestPoint(box, planes[i].Normal);
        float closestDist = Vector3DotProduct(closestPoint, planes[i].Normal) - planes[i].Distance;

        // If closest point is not fully inside, box is intersecting
        if (closestDist < 0)
            allInside = false;
    }

    // If all points passed all plane tests
    return allInside ? 1 : 0; // 1 = completely inside, 0 = partially inside/intersecting
}
