#pragma once

#include <d3d12.h>
#include <wrl.h>

using namespace Microsoft::WRL;

template <typename T>
class cUploadBuffer;

struct sObjectConstants;
struct sPassConstants;
struct sDirectionalLightConstants;
struct sMaterialConstants;

struct sFrameResource
{
	public:

		sFrameResource(ID3D12Device* _pDevice, UINT _passCount, UINT _objectCount, UINT _directionalLightCount, UINT _materialCount);
		~sFrameResource();

	public:

		ComPtr<ID3D12CommandAllocator> pCmdListAlloc;

		cUploadBuffer<sObjectConstants>*			pObjectCB;
		cUploadBuffer<sPassConstants>*				pPassCB;
		cUploadBuffer<sDirectionalLightConstants>*	pDirectLightCB;
		cUploadBuffer<sMaterialConstants>*			pMaterialCB;
		
		UINT64 fence;
};
