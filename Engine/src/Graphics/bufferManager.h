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
	// Transform
	XMFLOAT4X4 world;               // Object world transform
	XMFLOAT4X4 worldInvTranspose;   // For normal transforms

	// Material parameters
	XMFLOAT4 baseColor;             // RGB = Albedo, A = Opacity
	float metallicFactor;           // 0 = dielectric, 1 = metal
	float roughnessFactor;          // 0 = smooth, 1 = rough
	float aoFactor;                 // Ambient occlusion factor (1 = full AO)
	float padding;                  // padding

	XMFLOAT3 emissive;              // Emissive RGB
	float padding2;					// padding

	sObjectConstants()
	{
		XMStoreFloat4x4(&world, XMMatrixIdentity());
		XMStoreFloat4x4(&worldInvTranspose, XMMatrixIdentity());
		baseColor = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
		metallicFactor = 0.0f;
		roughnessFactor = 0.5f;
		aoFactor = 1.0f;
		emissive = XMFLOAT3(0.0f, 0.0f, 0.0f);
		padding = 0.0f;
		padding2 = 0.0f;
	}
};

struct sPassConstants
{
	XMFLOAT4X4	view;
	XMFLOAT4X4	invView; 
	XMFLOAT4X4	proj;
	XMFLOAT4X4	invProj;
	XMFLOAT4X4	viewProj; 
	XMFLOAT4X4	invViewProj;
	XMFLOAT3	eyePos; 
	int			lightCount;
	XMFLOAT2	renderTargetSize;
	XMFLOAT2	invRenderTargetSize;
	float		nearZ; 
	float		farZ;
	float		totalTime;
	float		deltaTime;
};

class cBufferManager
{
	public:

		cBufferManager(cDeviceManager* _pDeviceManager, cSwapChainManager* _pSwapChainManager);
		~cBufferManager();

	public:
		
		void Initialize(unsigned int _maxNumberOfRenderItems, unsigned int m_maxNumberOfLights);

	public:

		ID3D12DescriptorHeap* GetCbvHeap() const;

	private:

		void InitializeDescriptorHeaps();

	private:

		cDeviceManager*		m_pDeviceManager;
		cSwapChainManager*	m_pSwapChainManager;

		unsigned int m_maxNumberOfRenderItems; 
		unsigned int m_maxNumberOfLights; 

		ComPtr<ID3D12DescriptorHeap> m_pCbvHeap;
};
