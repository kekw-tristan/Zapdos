#include "directx12Util.h"

#include <filesystem>
#include <string>
#include <iostream>
#include <d3dx12.h>
#include <d3dcompiler.h>

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

ComPtr<ID3D12Resource> cDirectX12Util::CreateDefaultBuffer(ID3D12Device* _pDevice, ID3D12GraphicsCommandList* _pCmdList,
    const void* _pInitData, UINT64 _byteSize, ComPtr<ID3D12Resource> _pUploadBuffer)
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

    return pDefaultBuffer;
}

// --------------------------------------------------------------------------------------------------------------------------

UINT cDirectX12Util::CalculateBufferByteSize(UINT _bytesize)
{
    return (_bytesize + 255) & ~255;
}

// --------------------------------------------------------------------------------------------------------------------------

ComPtr<ID3DBlob> cDirectX12Util::CompileShader(const std::wstring& _rFilename, 
    const D3D_SHADER_MACRO* _pDefines, const std::string& _rEntrypoint, const std::string& _rTarget)
{
    UINT compileFlags = 0;

    #if defined(DEBUG) || defined(_DEBUG)
        compileFlags = D3DCOMPILE_DEBUG |
        D3DCOMPILE_SKIP_OPTIMIZATION;
    #endif

    ComPtr<ID3DBlob> pByteCode = nullptr;
    ComPtr<ID3DBlob> pErrors = nullptr;

    if (!std::filesystem::exists(_rFilename)) 
    {
        std::cout << "Absolute path: " << std::filesystem::absolute(_rFilename) << "\n";
        throw std::runtime_error("Shader file not found: " + std::string(_rFilename.begin(), _rFilename.end()));
    }

    HRESULT hr = D3DCompileFromFile(
        _rFilename.c_str(),
        _pDefines,
        D3D_COMPILE_STANDARD_FILE_INCLUDE,
        _rEntrypoint.c_str(),
        _rTarget.c_str(),
        compileFlags,
        0,
        &pByteCode,
        &pErrors
    );

    if (pErrors != nullptr)
    {
        std::string errorMsg = (char*)pErrors->GetBufferPointer();
        std::cout << "Shader Compilation Error: " << errorMsg << std::endl;
        OutputDebugStringA((char*)pErrors->GetBufferPointer());
    }
       
    ThrowIfFailed(hr);

    return pByteCode;
}

// --------------------------------------------------------------------------------------------------------------------------
