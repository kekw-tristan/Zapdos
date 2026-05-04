#include "pipelineStateManager.h"

#include <d3dx12.h>

#include "rootSignatureManager.h"
#include "shaderManager.h"

// --------------------------------------------------------------------------------------------------------------------------

cPipelineStateManager::cPipelineStateManager()
    : m_pDevice(nullptr)
    , m_pShaderManager(nullptr)
    , m_pRootSignatureManager(nullptr)
    , m_InputLayouts()
    , m_pipelineStateObjects()
{
}

// --------------------------------------------------------------------------------------------------------------------------

cPipelineStateManager::~cPipelineStateManager()
{
}

// --------------------------------------------------------------------------------------------------------------------------

void cPipelineStateManager::Initialize(ID3D12Device* _pDevice, cShaderManager* _pShaderManager, cRootSignatureManager* _pRootSignatureManager)
{
    m_pDevice               = _pDevice; 
    m_pShaderManager        = _pShaderManager; 
    m_pRootSignatureManager = _pRootSignatureManager;

    CreateGraphicsPSO();
    CreateMipGenPso(); 
}

// --------------------------------------------------------------------------------------------------------------------------

ID3D12PipelineState* cPipelineStateManager::GetPipelineState(const std::string& _rName)
{
    return m_pipelineStateObjects.at(_rName).Get();
}

// --------------------------------------------------------------------------------------------------------------------------

void cPipelineStateManager::CreateGraphicsPSO()
{
    m_InputLayouts =
    {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT,   0, 0,  D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "NORMAL",   0, DXGI_FORMAT_R32G32B32_FLOAT,   0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "TANGENT",  0, DXGI_FORMAT_R32G32B32A32_FLOAT,0, 24, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,      0, 40, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "TEXCOORD", 1, DXGI_FORMAT_R32G32_FLOAT,      0, 48, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
    };

    D3D12_GRAPHICS_PIPELINE_STATE_DESC desc = {};
    desc.pRootSignature = m_pRootSignatureManager->GetRootSignature("graphics");

    auto vs = m_pShaderManager->GetShader("vs");
    auto ps = m_pShaderManager->GetShader("ps");

    desc.VS = { vs->GetBufferPointer(), vs->GetBufferSize() };
    desc.PS = { ps->GetBufferPointer(), ps->GetBufferSize() };

    desc.RasterizerState    = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
    desc.BlendState         = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
    desc.DepthStencilState  = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);

    desc.SampleMask             = UINT_MAX;
    desc.PrimitiveTopologyType  = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    desc.NumRenderTargets       = 1;
    desc.RTVFormats[0]          = DXGI_FORMAT_R8G8B8A8_UNORM;
    desc.DSVFormat              = DXGI_FORMAT_D24_UNORM_S8_UINT;
    desc.SampleDesc.Count       = 1;
    desc.InputLayout            = { m_InputLayouts.data(), static_cast<UINT>(m_InputLayouts.size()) };

    ComPtr<ID3D12PipelineState> pso;
    m_pDevice->CreateGraphicsPipelineState(&desc, IID_PPV_ARGS(&pso));

    m_pipelineStateObjects["graphics"] = pso;
}

// --------------------------------------------------------------------------------------------------------------------------

void cPipelineStateManager::CreateMipGenPso()
{
    //D3D12_COMPUTE_PIPELINE_STATE_DESC desc = {};
    //desc.pRootSignature = m_pRootSignatureManager->GetRootSignature("mipgen");
    //
    //auto cs = m_pShaderManager->GetShader("mipgen_cs");
    //
    //desc.CS = { cs->GetBufferPointer(), cs->GetBufferSize() };
    //
    //ComPtr<ID3D12PipelineState> pso;
    //m_pDevice->CreateComputePipelineState(&desc, IID_PPV_ARGS(&pso));
    //
    //m_pipelineStateObjects["mipgen"] = pso;
}

// --------------------------------------------------------------------------------------------------------------------------