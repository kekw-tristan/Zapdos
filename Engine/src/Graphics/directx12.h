#pragma once

#include <d3d12.h>
#include <dxgi1_6.h>
#include <DirectXMath.h>
#include <vector>
#include <wrl.h>

using namespace DirectX;
using namespace Microsoft::WRL;

struct sVertex;
struct sMeshGeometry;

class cWindow;
class cTimer;

class cSwapChainManager;
class cDeviceManager;
class cBufferManager;
class cPipelineManager;

template<typename T>
class cUploadBuffer;

class cDirectX12
{
	public:

		cDirectX12()	= default;
		~cDirectX12()	= default;

		cDirectX12(const cDirectX12&) = delete;
		cDirectX12& operator=(const cDirectX12&) = delete;

	public:

		void Initialize(cWindow* _pWindow, cTimer* _pTimer);
		void Finalize();
		
	public:
		void InitializeVertices();
		void Update(XMMATRIX _view);
		void Draw(); 
		float GetAspectRatio() const;
		void CalculateFrameStats() const;
		void OnResize();

	private:

		cWindow* m_pWindow;
		cTimer*	 m_pTimer;

	private:

		sMeshGeometry* m_pBoxGeometry;
		
		XMFLOAT4X4 m_proj;
		XMFLOAT4X4 m_view;
		XMFLOAT4X4 m_world;

		cSwapChainManager* m_pSwapChainManager;
		cDeviceManager* m_pDeviceManager;
		cBufferManager* m_pBufferManager;
		cPipelineManager* m_pPipelineManager;

};
