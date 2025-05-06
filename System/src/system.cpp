#include "system.h"

#include <iostream>
#include <DirectXMath.h>
#include <DirectXColors.h>
#include <wrl.h>
#include <d3d12.h>

#include "window.h"
#include "directx12.h"
#include "directx12Util.h"
#include "timer.h"
#include "input.h"
#include "vertex.h"

using namespace DirectX;
using namespace Microsoft::WRL;

// --------------------------------------------------------------------------------------------------------------------------

void cSystem::Initialize()
{
	std::cout << "Initialize" << "\n";
	m_pTimer = new cTimer();
	m_pTimer->Start();
	
	m_pWindow = new cWindow();
	m_pWindow->Initialize(L"Zapdos", L"gameWindow", 1280, 720, m_pTimer);

	m_pDirectX12 = new cDirectX12();
	m_pDirectX12->Initialize(m_pWindow, m_pTimer);

	InitializeVertices();
}

// --------------------------------------------------------------------------------------------------------------------------

void cSystem::Run()
{
	while (m_pWindow->GetIsRunning())
	{
		m_pWindow->MessageHandling();

		// check if paused		
		if (m_pWindow->GetIsWindowPaused())
			continue;


		if (m_pWindow->GetHasResized())
		{
			m_pDirectX12->OnResize();
			m_pWindow->SetHasResized(false);
		}
		
		m_pTimer->Tick();

		m_pDirectX12->CalculateFrameStats();
		m_pDirectX12->Draw();
	}	

}

// --------------------------------------------------------------------------------------------------------------------------

void cSystem::Finalize()
{
	std::cout << "Finalize" << "\n";

	delete m_pWindow;
	delete m_pDirectX12;
	delete m_pTimer;
}

// --------------------------------------------------------------------------------------------------------------------------

void cSystem::InitializeVertices()
{
	// vertices	

	sVertex vertices[] =
	{
		{ XMFLOAT3(-1.0f, -1.0f, -1.0f), XMFLOAT4(Colors::White)   },
		{ XMFLOAT3(-1.0f, +1.0f, -1.0f), XMFLOAT4(Colors::Black)   },
		{ XMFLOAT3(+1.0f, +1.0f, -1.0f), XMFLOAT4(Colors::Red)     },
		{ XMFLOAT3(+1.0f, -1.0f, -1.0f), XMFLOAT4(Colors::Green)   },
		{ XMFLOAT3(-1.0f, -1.0f, +1.0f), XMFLOAT4(Colors::Blue)    },
		{ XMFLOAT3(-1.0f, +1.0f, +1.0f), XMFLOAT4(Colors::Yellow)  },
		{ XMFLOAT3(+1.0f, +1.0f, +1.0f), XMFLOAT4(Colors::Cyan)    },
		{ XMFLOAT3(+1.0f, -1.0f, +1.0f), XMFLOAT4(Colors::Magenta) }
	};

	
	const UINT64 vbByteSize = 8 * sizeof(sVertex);

	ComPtr<ID3D12Resource> pVertexBufferGPU = nullptr;
	ComPtr<ID3D12Resource> pVertexBufferUploader = nullptr;

	pVertexBufferGPU = cDirectX12Util::CreateDefaultBuffer(m_pDirectX12->GetDevice().Get(), m_pDirectX12->GetCommandList().Get(), vertices, vbByteSize, pVertexBufferUploader);

	D3D12_VERTEX_BUFFER_VIEW vbv;

	vbv.BufferLocation = pVertexBufferGPU->GetGPUVirtualAddress();
	vbv.StrideInBytes = sizeof(sVertex);
	vbv.SizeInBytes = 8 * sizeof(sVertex);

	D3D12_VERTEX_BUFFER_VIEW vertexBuffers[1] = { vbv };
	m_pDirectX12->GetCommandList()->IASetVertexBuffers(0, 1, vertexBuffers);

	// indices

	std::uint16_t indices[] = {
		// front face
		0, 1, 2,
		0, 2, 3,
		// back face
		4, 6, 5,
		4, 7, 6,
		// left face
		4, 5, 1,
		4, 1, 0,
		// right face
		3, 2, 6,
		3, 6, 7,
		// top face
		1, 5, 6,
		1, 6, 2,
		// bottom face
		4, 0, 3,
		4, 3, 7
	};

	const UINT ibByteSize = 36 * sizeof(std::uint16_t);

	ComPtr<ID3D12Resource> pIndexBufferGPU = nullptr;
	ComPtr<ID3D12Resource> pIndexBufferUploader = nullptr;

	pIndexBufferGPU = cDirectX12Util::CreateDefaultBuffer(m_pDirectX12->GetDevice().Get(), m_pDirectX12->GetCommandList().Get(), indices, ibByteSize, pIndexBufferUploader);

	D3D12_INDEX_BUFFER_VIEW ibv;

	ibv.BufferLocation = pIndexBufferGPU->GetGPUVirtualAddress();
	ibv.Format = DXGI_FORMAT_R16_UINT;
	ibv.SizeInBytes = ibByteSize;

	m_pDirectX12->GetCommandList()->IASetIndexBuffer(&ibv);
}

// --------------------------------------------------------------------------------------------------------------------------
