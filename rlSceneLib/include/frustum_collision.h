#pragma once

#include "raylib.h"
#include "raymath.h"

// Plane structure: defined by a normal vector and distance from origin
struct Plane
{
    Vector3 Normal;		// Normal vector (should be normalized)
    float Distance;		// Distance from origin along the normal
};

// Camera frustum containing 6 planes
struct CameraFrustum
{
    Plane Left;
    Plane Right;
    Plane Top;
    Plane Bottom;
    Plane Near;
    Plane Far;

};

struct ViewCamera
{
    Camera3D Camera;

    float NearPlaneDistance = 0.01f;
    float FarPlaneDistance = 1000.0f;

    CameraFrustum Frustum;

    void SetPlanes(float near, float far);
};

bool ExtractFrustumPlanes(ViewCamera& camera);

float GetSignedDistanceToPlane(const Plane& plane, const Vector3& point);
bool IsSphereInFrustum(const CameraFrustum& frustum, const Vector3& center, float radius);

bool IsBoxInFrustum(const CameraFrustum& frustum, const BoundingBox& box, Matrix transform = MatrixIdentity());