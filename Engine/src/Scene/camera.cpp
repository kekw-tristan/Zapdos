#include "camera.h"
#include <algorithm> 

// --------------------------------------------------------------------------------------------------------------------------

cCamera::cCamera()
    : m_position(0.f, 0.f, -30.f)
    , m_pitch(0.f)
    , m_yaw(0.f)
{
    XMMATRIX P = XMMatrixPerspectiveFovLH(XM_PI / 4.0f, 16.f / 9.f, 0.1f, 1000.f);
    XMStoreFloat4x4(&m_proj, P);
}

// --------------------------------------------------------------------------------------------------------------------------

void cCamera::SetPosition(const XMFLOAT3& pos)
{
    m_position = pos;
}

// --------------------------------------------------------------------------------------------------------------------------

XMFLOAT3 cCamera::GetPosition() const
{
    return m_position;
}

// --------------------------------------------------------------------------------------------------------------------------

void cCamera::SetRotation(float pitch, float yaw)
{
    m_pitch = pitch;
    m_yaw = yaw;
}

// --------------------------------------------------------------------------------------------------------------------------

void cCamera::GetRotation(float& pitch, float& yaw) const
{
    pitch = m_pitch;
    yaw = m_yaw;
}

// --------------------------------------------------------------------------------------------------------------------------

void cCamera::Move(const XMFLOAT3& delta)
{
    XMMATRIX rot = XMMatrixRotationRollPitchYaw(m_pitch, m_yaw, 0.f);
    XMVECTOR d = XMLoadFloat3(&delta);
    XMVECTOR pos = XMLoadFloat3(&m_position);

    pos += XMVector3TransformCoord(d, rot);

    XMStoreFloat3(&m_position, pos);
}

// --------------------------------------------------------------------------------------------------------------------------

void cCamera::Rotate(float dPitch, float dYaw)
{
    m_pitch += dPitch;
    m_yaw += dYaw;

    // clamp pitch to avoid flipping
    m_pitch = std::clamp(m_pitch, -XM_PIDIV2 + 0.001f, XM_PIDIV2 - 0.001f);
}

// --------------------------------------------------------------------------------------------------------------------------

XMMATRIX cCamera::GetViewMatrix() const
{
    XMVECTOR pos = XMLoadFloat3(&m_position);
    XMMATRIX rot = XMMatrixRotationRollPitchYaw(m_pitch, m_yaw, 0.f);

    XMVECTOR forward = XMVector3TransformCoord(XMVectorSet(0, 0, 1, 0), rot);
    XMVECTOR up = XMVector3TransformCoord(XMVectorSet(0, 1, 0, 0), rot);

    return XMMatrixLookAtLH(pos, pos + forward, up);
}

// --------------------------------------------------------------------------------------------------------------------------

XMMATRIX cCamera::GetProjectionMatrix() const
{
    return XMLoadFloat4x4(&m_proj);
}

// --------------------------------------------------------------------------------------------------------------------------

void cCamera::SetProjection(float fovY, float aspect, float nearZ, float farZ)
{
    XMMATRIX P = XMMatrixPerspectiveFovLH(fovY, aspect, nearZ, farZ);
    XMStoreFloat4x4(&m_proj, P);
}

// --------------------------------------------------------------------------------------------------------------------------