#pragma once 

#include <cstdint>
#include <windows.h>
#include <DirectXMath.h>
#include <vector>
#include <memory>

#include <graphics/renderItem.h>
#include <graphics/light.h>
#include <Scene/scene.h>

using namespace DirectX;

class cWindow;
class cDirectX12;
class cTimer;

constexpr int c_numberOfRenderItems = 10000;

class cSystem
{
    public:
        cSystem()  = default;
        ~cSystem() = default;
    
    public:
        void Initialize();
        void Run();
        void Finalize();
    
    private:
        void InitializeRenderItems();
        void InitializeLights();
    
        void Update();
        void HandleInput();
    
    private:
        cWindow* m_pWindow = nullptr;
        cDirectX12* m_pDirectX12 = nullptr;
        cTimer* m_pTimer = nullptr;
    
        std::unique_ptr<cScene> m_pScene;
    
        XMFLOAT4X4 m_view{};
        XMFLOAT4X4 m_world{};
        XMFLOAT4X4 m_proj{};
    
        XMFLOAT3 m_position = XMFLOAT3(0.0f, 0.0f, -30.0f);
        float    m_yaw = 0.0f;
        float    m_pitch = 0.0f;
};