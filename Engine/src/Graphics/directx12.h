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
struct sFrameResource; 

class cWindow;
class cTimer;

class cSwapChainManager;
class cDeviceManager;
class cBufferManager;
class cPipelineManager;

class cRenderItem; 

template<typename T>
class cUploadBuffer;

constexpr int c_NumberOfFrameResources = 3; 

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

		void InitializeFrameResources();
		void WaitForCurrentFrameResourceIfInUse(); 
		void WaitForGPU();

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

		std::vector<sFrameResource*>	m_frameResources;
		std::vector<cRenderItem*>		m_renderItems;
		sFrameResource* m_pCurrentFrameResource;
		int m_currentFrameResourceIndex; 
};
