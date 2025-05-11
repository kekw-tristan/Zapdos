#pragma once

#include <DirectXMath.h>
#include <DirectXCollision.h>
#include <d3d12.h>
#include <string>
#include <wrl.h>
#include <unordered_map>

using namespace DirectX;
using namespace Microsoft::WRL;

struct sSubmeshGeometry
{
	UINT indexCount				= 0;
	UINT startIndexLocation		= 0;
	INT  startVertexLocation	= 0;

	BoundingBox bounds;
};

struct sMeshGeometry
{
	std::string name;

	ComPtr<ID3DBlob>		vertexBufferCPU		= nullptr;
	ComPtr<ID3DBlob>		indexBufferCPU		= nullptr;

	ComPtr<ID3D12Resource> vertexBufferGPU		= nullptr;
	ComPtr<ID3D12Resource> indexBufferGPU		= nullptr;

	ComPtr<ID3D12Resource> vertexBufferUploader	= nullptr;
	ComPtr<ID3D12Resource> indexBufferUploader	= nullptr;

	UINT vertexByteStride		= 0;
	UINT vertexBufferByteSize	= 0;

	DXGI_FORMAT indexFormat		= DXGI_FORMAT_R16_UINT;
	UINT indexBufferByteSize	= 0;

	std::unordered_map<std::string, sSubmeshGeometry> drawArguments;

	D3D12_VERTEX_BUFFER_VIEW GetVertexBufferView() const
	{
		D3D12_VERTEX_BUFFER_VIEW vbv;

		vbv.BufferLocation	= vertexBufferGPU->GetGPUVirtualAddress();
		vbv.SizeInBytes		= vertexBufferByteSize;

		return vbv;
	}

	D3D12_INDEX_BUFFER_VIEW GetIndexBufferView() const
	{
		D3D12_INDEX_BUFFER_VIEW ibv;

		ibv.BufferLocation	= indexBufferGPU->GetGPUVirtualAddress();
		ibv.Format			= indexFormat;
		ibv.SizeInBytes		= indexBufferByteSize;

		return ibv;
	}

	void DisposeUploaders()
	{
		vertexBufferUploader	= nullptr;
		indexBufferUploader		= nullptr;
	}
};