#include "pipelineManager.h"

#include <d3dx12.h>

#include "deviceManager.h"
#include "directx12Util.h"
#include "swapChainManager.h"

cPipelineManager::cPipelineManager(cDeviceManager* _pDeviceManager)
    : m_pDeviceManager(_pDeviceManager)
    , m_pRootSignature(nullptr)
{
}

void cPipelineManager::Initialize()
{
    InitializeRootSignature();
    InitializeShader();
    InitializePipelineStateObject();
}

// --------------------------------------------------------------------------------------------------------------------------

ID3D12PipelineState* cPipelineManager::GetPipelineStateObject() const
{
    return m_pPso.Get();
}

// --------------------------------------------------------------------------------------------------------------------------

ID3D12RootSignature* cPipelineManager::GetRootSignature() const
{
    return m_pRootSignature.Get();
}

// --------------------------------------------------------------------------------------------------------------------------

void cPipelineManager::InitializeRootSignature()
{
    CD3DX12_ROOT_PARAMETER slotRootParameter[3] = {};

    // per object 
    CD3DX12_DESCRIPTOR_RANGE cbvTable0;
    cbvTable0.Init(
        D3D12_DESCRIPTOR_RANGE_TYPE_CBV,
        1,
        0 // b0
    );
    slotRootParameter[0].InitAsDescriptorTable(
        1,
        &cbvTable0
    );

    // pass constants 
    CD3DX12_DESCRIPTOR_RANGE cbvTable1;
    cbvTable1.Init(
        D3D12_DESCRIPTOR_RANGE_TYPE_CBV,
        1,
        1 // b1
    );
    slotRootParameter[1].InitAsDescriptorTable(
        1,
        &cbvTable1
    );

    // lights 
    CD3DX12_DESCRIPTOR_RANGE srvTable0;
    srvTable0.Init(
        D3D12_DESCRIPTOR_RANGE_TYPE_SRV,
        1,
        0 // t0
    );
    slotRootParameter[2].InitAsDescriptorTable(
        1,
        &srvTable0
    );

    CD3DX12_ROOT_SIGNATURE_DESC rootSigDesc(
        3,                              // Number of root parameters
        slotRootParameter,              // Pointer to root parameters array
        0,                              // No static samplers
        nullptr,
        D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT
    );

    ComPtr<ID3DBlob> serializedRootSig = nullptr;
    ComPtr<ID3DBlob> errorBlob = nullptr;

    cDirectX12Util::ThrowIfFailed(D3D12SerializeRootSignature(
        &rootSigDesc,
        D3D_ROOT_SIGNATURE_VERSION_1,
        &serializedRootSig,
        &errorBlob
    ));

    cDirectX12Util::ThrowIfFailed(m_pDeviceManager->GetDevice()->CreateRootSignature(
        0,
        serializedRootSig->GetBufferPointer(),
        serializedRootSig->GetBufferSize(),
        IID_PPV_ARGS(&m_pRootSignature)
    ));
}

// --------------------------------------------------------------------------------------------------------------------------

void cPipelineManager::InitializeShader()
{
    m_pVsByteCode = cDirectX12Util::CompileShader(L"..\\Assets\\Shader\\shader.hlsl", nullptr, "VS", "vs_5_0");
    m_pPsByteCode = cDirectX12Util::CompileShader(L"..\\Assets\\Shader\\shader.hlsl", nullptr, "PS", "ps_5_0");

    m_InputLayouts =
    {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT,   0, 0,  D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "NORMAL",   0, DXGI_FORMAT_R32G32B32_FLOAT,   0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "TANGENT",  0, DXGI_FORMAT_R32G32B32A32_FLOAT,0, 24, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,      0, 40, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "TEXCOORD", 1, DXGI_FORMAT_R32G32_FLOAT,      0, 48, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
    };
}

// --------------------------------------------------------------------------------------------------------------------------

void cPipelineManager::InitializePipelineStateObject()
{
    D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {}; 

    psoDesc.InputLayout             = { m_InputLayouts.data(), static_cast<UINT>(m_InputLayouts.size()) };                              // Set input layout for vertex data
    psoDesc.pRootSignature          = m_pRootSignature.Get();                                                                           // Assign root signature used by the shaders
    psoDesc.VS                      = { reinterpret_cast<BYTE*>(m_pVsByteCode->GetBufferPointer()), m_pVsByteCode->GetBufferSize() };   // Set compiled vertex shader bytecode
    psoDesc.PS                      = { reinterpret_cast<BYTE*>(m_pPsByteCode->GetBufferPointer()), m_pPsByteCode->GetBufferSize() };   // Set compiled pixel shader bytecode
    psoDesc.RasterizerState         = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);      // Use default rasterizer state (solid fill, backface culling)
    psoDesc.RasterizerState.FrontCounterClockwise = TRUE;
    psoDesc.BlendState              = CD3DX12_BLEND_DESC(D3D12_DEFAULT);                                                                // Use default blend state (no blending)
    psoDesc.DepthStencilState       = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);                                                        // Use default depth/stencil state (depth testing enabled)
    psoDesc.SampleMask              = UINT_MAX;                                                                                         // Sample mask (all samples enabled)
    psoDesc.PrimitiveTopologyType   = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;                                                           // Use triangle primitive topology
    psoDesc.NumRenderTargets        = 1;                                                                                                // One render target
    psoDesc.RTVFormats[0]           = c_backBufferFormat;                                                                               // Set format for render target view (RTV)
    psoDesc.SampleDesc.Count        = m_pDeviceManager->Get4xMSAAQuality() ? 4 : 1;                                                     // Set sample count (4 for MSAA if supported, otherwise 1)
    psoDesc.SampleDesc.Quality      = m_pDeviceManager->Get4xMSAAQuality() ? (m_pDeviceManager->Get4xMSAAQuality() - 1) : 0;            // Set sample quality level for MSAA
    psoDesc.DSVFormat               = c_depthStencilFormat;                                                                             // Set format for depth/stencil view (DSV)

    cDirectX12Util::ThrowIfFailed(m_pDeviceManager->GetDevice()->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&m_pPso)));         // Create the PSO using the device
}
