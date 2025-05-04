#include "util.h"

#include <string>
#include <iostream>
#include <d3dx12.h>

// --------------------------------------------------------------------------------------------------------------------------

inline void ThrowIfFailed(HRESULT hr)
{
    if (FAILED(hr))
    {

        std::cout << "DirectX call failed. HRESULT = " << std::to_string(hr) << "\n";

        throw std::runtime_error("DirectX call failed. HRESULT = " + std::to_string(hr));
    }
}

// --------------------------------------------------------------------------------------------------------------------------

ComPtr<ID3D12Resource> cUtil::CreateDefaultBuffer(ID3D12Device* _pDevice, ID3D12GraphicsCommandList* _pCmdList, const void* _pInitData, UINT64 _byteSize, ComPtr<ID3D12Resource> _pUploadBuffer)
{
    ComPtr<ID3D12Resource> pDefaultBuffer;

    // create default buffer resource
    ThrowIfFailed(_pDevice->CreateCommittedResource(
        &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
        D3D12_HEAP_FLAG_NONE,
        &CD3DX12_RESOURCE_DESC::Buffer(_byteSize),
        D3D12_RESOURCE_STATE_COMMON,
        nullptr,
        IID_PPV_ARGS(pDefaultBuffer.GetAddressOf())
    ));

    // ubterneduate upload heap
    ThrowIfFailed(_pDevice->CreateCommittedResource(
        &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
        D3D12_HEAP_FLAG_NONE,
        &CD3DX12_RESOURCE_DESC::Buffer(_byteSize),
        D3D12_RESOURCE_STATE_GENERIC_READ,
        nullptr,
        IID_PPV_ARGS(_pUploadBuffer.GetAddressOf())
    ));

    // describe data for default buffer
    D3D12_SUBRESOURCE_DATA subResourceData = {};

    subResourceData.pData       = _pInitData;
    subResourceData.RowPitch    = _byteSize;
    subResourceData.SlicePitch  = subResourceData.RowPitch;

    _pCmdList->ResourceBarrier(1,
        &CD3DX12_RESOURCE_BARRIER::Transition(
            pDefaultBuffer.Get(),
            D3D12_RESOURCE_STATE_COMMON,
            D3D12_RESOURCE_STATE_COPY_DEST
        ));

    UpdateSubresources<1>(
        _pCmdList,
        pDefaultBuffer.Get(),
        _pUploadBuffer.Get(),
        0, 0, 1,
        &subResourceData
    );

    _pCmdList->ResourceBarrier(1,
        &CD3DX12_RESOURCE_BARRIER::Transition(pDefaultBuffer.Get(),
            D3D12_RESOURCE_STATE_COPY_DEST,
            D3D12_RESOURCE_STATE_GENERIC_READ
    ));
}
