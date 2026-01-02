#pragma once 

#include <cstdint>
#include <windows.h>
#include <DirectXMath.h>
#include <vector>
#include <memory>

#include <graphics/renderItem.h>
#include <graphics/light.h>
#include <Scene/scene.h>
#include "Scene/camera.h"

using namespace DirectX;

class cWindow;
class cDirectX12;
class cTimer;

constexpr int c_numberOfRenderItems = 10000;

class cSystem
{
    public:

        cSystem() = default;
        ~cSystem() = default;
    
    public:

        void Initialize();
        void Run();
        void Finalize();
    
    private:

        void InitializeRenderItems();
        void InitializeLights();
    
        void Update(float deltaTime);
        void HandleInput(float deltaTime);
    
    private:

        cWindow* m_pWindow = nullptr;
        cDirectX12* m_pDirectX12 = nullptr;
        cTimer* m_pTimer = nullptr;
    
        std::unique_ptr<cScene>  m_pScene;
        std::unique_ptr<cCamera> m_pCamera;
    
        XMFLOAT4X4 m_view{};
        XMFLOAT4X4 m_proj{};
};