#pragma once

#include <d3d12.h>
#include <DirectXMath.h>
#include <dxgi1_6.h>
#include <wrl.h>

using namespace Microsoft::WRL;

class cDeviceManager;
class cSwapChainManager;

template<typename T>
class cUploadBuffer;

class cBufferManager
{
	public:

		cBufferManager(cDeviceManager* _pDeviceManager, cSwapChainManager* _pSwapChainManager);
		~cBufferManager();

	public:

		struct sObjectConstants
		{
			DirectX::XMFLOAT4X4 worldViewProj;
			sObjectConstants() { DirectX::XMStoreFloat4x4(&worldViewProj, DirectX::XMMatrixIdentity()); }
		};

	public:
		
		void Initialize();

	public:

		ID3D12DescriptorHeap* GetCbvHeap()				const;
		cUploadBuffer<sObjectConstants>* GetObjectCB()	const;

	private:

		void InitializeDescriptorHeaps();
		void InitializeConstantBuffer();

	private:

		cDeviceManager*		m_pDeviceManager;
		cSwapChainManager*	m_pSwapChainManager;

		cUploadBuffer<sObjectConstants>* m_pObjectCB;
		ComPtr<ID3D12DescriptorHeap> m_pCbvHeap;
};
