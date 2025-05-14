#pragma once

#include <d3d12.h>
#include <dxgi1_6.h>
#include <wrl.h>

using namespace Microsoft::WRL;

constexpr int c_swapChainBufferCount = 2;
constexpr DXGI_FORMAT c_depthStencilFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;

class cDeviceManager;
class cWindow;

class cSwapChainManager
{
	public:

		cSwapChainManager(cDeviceManager* _pDeviceManager, cWindow* _pWindow);

	public:

		void Initialize();
		
	public:

		IDXGISwapChain4* GetSwapChain();

	private:

		void InitializeSwapChain();
		void InitializeDescriptorHeaps();
		void InitializeRenderTargetView();
		void InitializeDepthStencilView();

	private:
	
		cDeviceManager* m_pDeviceManager;
		cWindow*		m_pWindow;

		ComPtr<IDXGISwapChain4> m_pSwapChain;

		ComPtr<ID3D12DescriptorHeap> m_pRtvHeap;
		ComPtr<ID3D12DescriptorHeap> m_pDsvHeap;

		D3D12_VIEWPORT m_viewPort;

		ComPtr<ID3D12Resource> m_pSwapChainBuffer[c_swapChainBufferCount];
		ComPtr<ID3D12Resource> m_pDepthStencilBuffer;
};
