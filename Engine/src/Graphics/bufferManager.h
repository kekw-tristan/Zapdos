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

using namespace DirectX; 

struct sObjectConstants
{
	XMFLOAT4X4 world;
	sObjectConstants() { XMStoreFloat4x4(&world, XMMatrixIdentity()); }
};

struct sPassConstants
{
	XMFLOAT4X4 view;
	XMFLOAT4X4 invView; 
	XMFLOAT4X4 proj;
	XMFLOAT4X4 invProj;
	XMFLOAT4X4 viewProj; 
	XMFLOAT4X4 invViewProj;
	XMFLOAT3 eyePos; 
	float cbPerObjectPad1;
	XMFLOAT2 renderTargetSize;
	XMFLOAT2 invRenderTargetSize;
	float nearZ; 
	float farZ;
	float totalTime;
	float deltaTime;
};

class cBufferManager
{
	public:

		cBufferManager(cDeviceManager* _pDeviceManager, cSwapChainManager* _pSwapChainManager);
		~cBufferManager();

	public:
		
		void Initialize();

	public:

		ID3D12DescriptorHeap* GetCbvHeap()				const;

	private:

		void InitializeDescriptorHeaps();

	private:

		cDeviceManager*		m_pDeviceManager;
		cSwapChainManager*	m_pSwapChainManager;

		ComPtr<ID3D12DescriptorHeap> m_pCbvHeap;
};
