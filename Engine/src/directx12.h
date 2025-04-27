#pragma once

#include <d3d12.h>
#include <dxgi1_4.h>
#include <wrl.h>

#pragma comment(lib, "d3d12.lib")
#pragma comment(lib, "dxgi.lib")

using namespace Microsoft::WRL;

int constexpr c_swapChainBufferCount = 2;

class cWindow;

class cDirectX12
{
	public:

		cDirectX12()	= default;
		~cDirectX12()	= default;

	public:

		void Initialize(cWindow* _pWindow);

	private:

		void InitializeDeviceAndFactory();
		void InitializeFenceAndDescriptor();
		void Check4XMSAAQualitySupport();
		void InitializeCommandQueueAndList();
		void InitializeSwapChain();
		void InitializeDescriptorHeaps();
		void InitializeRenderTargetView();
	
	private:

		D3D12_CPU_DESCRIPTOR_HANDLE GetCurrentBackbufferView() const; 
		D3D12_CPU_DESCRIPTOR_HANDLE GetDepthStencilView() const; 
	
	private:

		

	private:

		cWindow* m_pWindow;

	private:

		ComPtr<IDXGIFactory4>	m_pDxgiFactory;
		ComPtr<ID3D12Device>	m_pDevice;
		ComPtr<ID3D12Fence>		m_pFence;

		ComPtr<ID3D12CommandQueue>			m_pCommandQueue;
		ComPtr<ID3D12CommandAllocator>		m_pDirectCmdListAlloc;
		ComPtr<ID3D12GraphicsCommandList>	m_pCommandList;
		
		ComPtr<IDXGISwapChain> m_pSwapChain;

		ComPtr<ID3D12DescriptorHeap> m_pRtvHeap;
		ComPtr<ID3D12DescriptorHeap> m_pDsvHeap; 

		DXGI_FORMAT m_backBufferFormat; 

		int m_4xMsaaQuality; 

		int m_rtvDescriptorSize;
		int m_dsvDescriptorSize;
		int m_cbvSrvDescriptorSize;

		int m_currentBackBuffer;
		
};
