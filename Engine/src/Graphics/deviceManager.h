#pragma once

#include <d3d12.h>
#include <dxgi1_6.h>
#include <wrl.h>

using namespace Microsoft::WRL;

constexpr DXGI_FORMAT c_backBufferFormat = DXGI_FORMAT_R8G8B8A8_UNORM;

class cDeviceManager
{
	public:

		cDeviceManager();

	public:

		struct sDescriptorSizes
		{
			UINT cbvSrvUav;
			UINT rtv;
			UINT dsv;
			UINT sampler;
		};

	public:

		void Initialize();

	public:

		void FlushCommandQueue();

	public:

		ID3D12Device*				GetDevice()				const;
		IDXGIFactory7*				GetDxgiFactory()		const;
		ID3D12CommandQueue*			GetCommandQueue()		const;
		ID3D12Fence*				GetFence()				const;
		ID3D12GraphicsCommandList*	GetCommandList()		const;
		ID3D12CommandAllocator*		GetDirectCmdListAlloc() const;
		const sDescriptorSizes&		GetDescriptorSizes()	const;
		int							Get4xMSAAQuality()		const;

	

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

		int m_currentFence;

};