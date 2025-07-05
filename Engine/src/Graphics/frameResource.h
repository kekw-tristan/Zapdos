#pragma once

#include <d3d12.h>
#include <wrl.h>

using namespace Microsoft::WRL;

template <typename T>
class cUploadBuffer;

struct sObjectConstants;
struct sPassConstants;
struct sLightConstants;
struct sMaterialConstants;

struct sFrameResource
{
	public:

		sFrameResource(ID3D12Device* _pDevice, UINT _passCount, UINT _objectCount, UINT _lightCount);
		~sFrameResource();

	public:

		ComPtr<ID3D12CommandAllocator> pCmdListAlloc;

		cUploadBuffer<sObjectConstants>*			pObjectCB;
		cUploadBuffer<sPassConstants>*				pPassCB;
		cUploadBuffer<sLightConstants>*				pLightBuffer;
		cUploadBuffer<sMaterialConstants>*			pMaterialCB;
		
		UINT64 fence;
};
