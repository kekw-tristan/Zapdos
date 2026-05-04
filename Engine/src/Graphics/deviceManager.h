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

		ID3D12Device*				GetDevice()				const;
		IDXGIFactory7*				GetDxgiFactory()		const;
		const sDescriptorSizes&		GetDescriptorSizes()	const;
		int							Get4xMSAAQuality()		const;

	private:

		void InitializeDeviceAndFactory(); 
		void InitializeDescriptorSize();
		void Check4XMSAAQualitySupport();

	private:

		ComPtr<ID3D12Device>				m_pDevice;
		ComPtr<IDXGIFactory7>				m_pDxgiFactory;

		sDescriptorSizes m_descriptorSizes;

		int m_4xMsaaQuality;
		int m_currentFence;

};