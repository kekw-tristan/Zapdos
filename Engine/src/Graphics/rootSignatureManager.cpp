#include "rootSignatureManager.h"

#include <d3dx12.h>

// --------------------------------------------------------------------------------------------------------------------------

cRootSignatureManager::cRootSignatureManager()
    : m_pDevice(nullptr)
    , m_rootSignatures()
{
}

// --------------------------------------------------------------------------------------------------------------------------

cRootSignatureManager::~cRootSignatureManager()
{
}

// --------------------------------------------------------------------------------------------------------------------------

void cRootSignatureManager::Initialize()
{
    CreateGraphicsRS();
    CreateMipGenRS();
}

// --------------------------------------------------------------------------------------------------------------------------

ID3D12RootSignature* cRootSignatureManager::GetRootSignature(const std::string& _rName)
{
    return m_rootSignatures.at(_rName).Get();
}

// --------------------------------------------------------------------------------------------------------------------------

void cRootSignatureManager::CreateGraphicsRS()
{
  
    CD3DX12_ROOT_PARAMETER params[4];

    CD3DX12_DESCRIPTOR_RANGE cbv0;
    cbv0.Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 0);
    params[0].InitAsDescriptorTable(1, &cbv0);

    CD3DX12_DESCRIPTOR_RANGE cbv1;
    cbv1.Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 1);
    params[1].InitAsDescriptorTable(1, &cbv1);

    CD3DX12_DESCRIPTOR_RANGE srv0;
    srv0.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0);
    params[2].InitAsDescriptorTable(1, &srv0);

    CD3DX12_DESCRIPTOR_RANGE srv1;
    srv1.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 512, 1);
    params[3].InitAsDescriptorTable(1, &srv1, D3D12_SHADER_VISIBILITY_PIXEL);

    CD3DX12_STATIC_SAMPLER_DESC samp(0, D3D12_FILTER_MIN_MAG_MIP_LINEAR);

    CD3DX12_ROOT_SIGNATURE_DESC desc(
        4, params,
        1, &samp,
        D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT
    );

    ComPtr<ID3DBlob> blob;
    ComPtr<ID3DBlob> err;

    D3D12SerializeRootSignature(&desc, D3D_ROOT_SIGNATURE_VERSION_1, &blob, &err);

    ComPtr<ID3D12RootSignature> rs;
    m_pDevice->CreateRootSignature(
        0,
        blob->GetBufferPointer(),
        blob->GetBufferSize(),
        IID_PPV_ARGS(&rs)
    );

    m_rootSignatures["graphics"] = rs;
}

// --------------------------------------------------------------------------------------------------------------------------

void cRootSignatureManager::CreateMipGenRS()
{
    CD3DX12_ROOT_PARAMETER params[2];

    params[0].InitAsShaderResourceView(0);   // t0
    params[1].InitAsUnorderedAccessView(0);  // u0

    CD3DX12_ROOT_SIGNATURE_DESC desc(
        2, params,
        0, nullptr
    );

    ComPtr<ID3DBlob> blob;
    ComPtr<ID3DBlob> err;

    D3D12SerializeRootSignature(&desc, D3D_ROOT_SIGNATURE_VERSION_1, &blob, &err);

    ComPtr<ID3D12RootSignature> rs;
    m_pDevice->CreateRootSignature(
        0,
        blob->GetBufferPointer(),
        blob->GetBufferSize(),
        IID_PPV_ARGS(&rs)
    );

    m_rootSignatures["mipgen"] = rs;
}

// --------------------------------------------------------------------------------------------------------------------------