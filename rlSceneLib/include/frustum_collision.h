#pragma once

#include "raylib.h"
#include "raymath.h"

// Plane structure for frustum culling
struct Plane
{
    Vector3 Normal;
    float Distance;
};

// Check if a bounding box is inside or intersects the camera frustum
// Returns:
//   -1 if completely outside
//    0 if partially inside (intersecting)
//    1 if completely inside
int CheckBoundingBoxFrustum(const BoundingBox& box, const Camera3D& camera);

// Helper function to extract frustum planes from camera
// Returns array of 6 planes (left, right, top, bottom, near, far)
void GetFrustumPlanes(const Camera3D& camera, Plane planes[6]);

// Helper function to get a point on the bounding box that is furthest in a given direction
Vector3 GetBoxFurthestPoint(const BoundingBox& box, const Vector3& direction);

// Helper function to get a point on the bounding box that is closest in a given direction
Vector3 GetBoxClosestPoint(const BoundingBox& box, const Vector3& direction);
