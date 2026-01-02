#pragma once
#include <DirectXMath.h>

using namespace DirectX;

class cCamera
{
    public:

        cCamera();
        ~cCamera() = default;
    
        void SetPosition(const XMFLOAT3& pos);
        XMFLOAT3 GetPosition() const;
    
        void SetRotation(float pitch, float yaw);
        void GetRotation(float& pitch, float& yaw) const;
    
        void Move(const XMFLOAT3& delta);    
        void Rotate(float dPitch, float dYaw);  
    
        XMMATRIX GetViewMatrix() const;
        XMMATRIX GetProjectionMatrix() const;
    
        void SetProjection(float fovY, float aspect, float nearZ, float farZ);
    
    private:

        XMFLOAT3 m_position;
        float    m_pitch;
        float    m_yaw;
        XMFLOAT4X4 m_proj;
};