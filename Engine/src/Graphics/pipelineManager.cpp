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
    // Define the root parameter array     
    CD3DX12_ROOT_PARAMETER slotRootParameter[2] = {};


    // Define a descriptor range for a Constant Buffer View (CBV)
    CD3DX12_DESCRIPTOR_RANGE cbvTable0;
    cbvTable0.Init(
        D3D12_DESCRIPTOR_RANGE_TYPE_CBV, // Type: Constant Buffer View
        1,                               // Number of descriptors in the range (1 CBV)
        0                                // Base shader register (register b0)
    );

    CD3DX12_DESCRIPTOR_RANGE cbvTable1;

    cbvTable1.Init(
        D3D12_DESCRIPTOR_RANGE_TYPE_CBV,
        1,
        1
    );

    // Initialize the root parameter as a descriptor table
    slotRootParameter[0].InitAsDescriptorTable(
        1,              // Number of descriptor ranges in the table (only one range)
        &cbvTable0       // Pointer to the descriptor range
    );

    slotRootParameter[1].InitAsDescriptorTable(
        1,
        &cbvTable1
    );

    // Create a root signature descriptor with the above root parameter
    CD3DX12_ROOT_SIGNATURE_DESC rootSigDesc(
        2,                              // Number of root parameters 
        slotRootParameter,              // Pointer to root parameters
        0,                              // Number of static samplers (none in this case)
        nullptr,                        // Pointer to static samplers (nullptr since none)
        D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT // Allow input layout for vertex data
    );

    // Serialize the root signature to a binary blob
    ComPtr<ID3DBlob> serializedRootSig = nullptr;
    ComPtr<ID3DBlob> errorBlob = nullptr;

    cDirectX12Util::ThrowIfFailed(D3D12SerializeRootSignature(
        &rootSigDesc,                       // Pointer to the root signature descriptor
        D3D_ROOT_SIGNATURE_VERSION_1,       // Root signature version
        &serializedRootSig,                 // Output serialized signature
        &errorBlob                          // Output error messages if any
    ));

    // Create the actual root signature object on the GPU device
    cDirectX12Util::ThrowIfFailed(m_pDeviceManager->GetDevice()->CreateRootSignature(
        0,                                      // Node mask (0 for single-GPU)
        serializedRootSig->GetBufferPointer(),  // Serialized signature bytecode
        serializedRootSig->GetBufferSize(),     // Size of the bytecode
        IID_PPV_ARGS(&m_pRootSignature)         // Output root signature object
    ));
}

// --------------------------------------------------------------------------------------------------------------------------

void cPipelineManager::InitializeShader()
{
    m_pVsByteCode = cDirectX12Util::CompileShader(L"..\\Assets\\Shader\\shader.hlsl", nullptr, "VS", "vs_5_0");
    m_pPsByteCode = cDirectX12Util::CompileShader(L"..\\Assets\\Shader\\shader.hlsl", nullptr, "PS", "ps_5_0");

    m_InputLayouts =
    {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT,    0, 0,  D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "COLOR",    0, DXGI_FORMAT_R32G32B32A32_FLOAT,  0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
    };
}

// --------------------------------------------------------------------------------------------------------------------------

void cPipelineManager::InitializePipelineStateObject()
{
    D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {}; // Zero-initialize the PSO descriptor

    psoDesc.InputLayout             = { m_InputLayouts.data(), static_cast<UINT>(m_InputLayouts.size()) };                              // Set input layout for vertex data
    psoDesc.pRootSignature          = m_pRootSignature.Get();                                                                           // Assign root signature used by the shaders
    psoDesc.VS                      = { reinterpret_cast<BYTE*>(m_pVsByteCode->GetBufferPointer()), m_pVsByteCode->GetBufferSize() };   // Set compiled vertex shader bytecode
    psoDesc.PS                      = { reinterpret_cast<BYTE*>(m_pPsByteCode->GetBufferPointer()), m_pPsByteCode->GetBufferSize() };   // Set compiled pixel shader bytecode
    psoDesc.RasterizerState         = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);                                                           // Use default rasterizer state (solid fill, backface culling)
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
