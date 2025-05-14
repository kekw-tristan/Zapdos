#pragma once

#include <d3d12.h>
#include <dxgi1_6.h>
#include <wrl.h>

using namespace Microsoft::WRL;

constexpr DXGI_FORMAT c_backBufferFormat = DXGI_FORMAT_R8G8B8A8_UNORM;


struct sDescriptorSizes
{
	UINT cbvSrvUav;
	UINT rtv;
	UINT dsv;
	UINT sampler;
};

class cDeviceManager
{
	public:

		cDeviceManager();

	public:

		void Initialize();

	public:

		ID3D12Device*		GetDevice();
		IDXGIFactory7*		GetDxgiFactory();
		ID3D12CommandQueue* GetCommandQueue();
		ID3D12Fence*		GetFence();
		sDescriptorSizes*	GetDescriptorSizes();
		int					Get4xMSAAQuality();

	

	private:

		void InitializeDeviceAndFactory(); 
		void InitializeFenceAndDescriptorSize();
		void Check4XMSAAQualitySupport();
		void InitializeCommandQueueAndList(); 

	private:

		ComPtr<ID3D12Device>				m_pDevice;
		ComPtr<IDXGIFactory7>				m_pDxgiFactory;
		ComPtr<ID3D12CommandQueue>			m_pCommandQueue;
		ComPtr<ID3D12CommandAllocator>		m_pDirectCmdListAlloc;
		ComPtr<ID3D12GraphicsCommandList>	m_pCommandList;
		ComPtr<ID3D12Fence>					m_pFence;

		HANDLE m_pFenceEvent;
		UINT64 m_fenceValue;

		sDescriptorSizes m_descriptorSizes;

		int m_4xMsaaQuality;

};