#pragma once

#include <vector>
#include <d3d12.h>

#include "gpuTexture.h"

class cCpuTexture;
class cCommandContext;

class cTextureManager
{
    public:

        cTextureManager()  = default;
        ~cTextureManager() = default;

    public:

        int UploadCpuTextures(
            std::vector<cCpuTexture>&   _rCpuTextures,
            ID3D12Device*               _pDevice,
            ID3D12DescriptorHeap*       _pHeap,
            cCommandContext*            _pCommandContext,
            ID3D12PipelineState*        _pMipGenPipelineState,
            ID3D12RootSignature*        _pMipGenRootSignature,
            UINT                        _textureSrvBaseOffset
        );

        void ReleaseUploadHeaps();

        const std::vector<cGpuTexture>& GetTextures() const noexcept
        {
            return m_textures;
        }

    private:

        D3D12_CPU_DESCRIPTOR_HANDLE GetCpuHandle(
            ID3D12DescriptorHeap*   _pHeap,
            UINT                    _descriptorSize,
            UINT                    _heapIndex
        ) const;

        D3D12_GPU_DESCRIPTOR_HANDLE GetGpuHandle(
            ID3D12DescriptorHeap*   _pHeap,
            UINT                    _descriptorSize,
            UINT                    _heapIndex
        ) const;

        D3D12_GPU_DESCRIPTOR_HANDLE CreateSRV(
            ID3D12Device*           _pDevice,
            ID3D12DescriptorHeap*   _pHeap,
            ID3D12Resource*         _pTexture,
            UINT                    _descriptorSize,
            UINT                    _heapIndex
        ) const;

        std::vector<D3D12_GPU_DESCRIPTOR_HANDLE> CreateMipUAVs(
            ID3D12Device*           _pDevice,
            ID3D12DescriptorHeap*   _pHeap,
            ID3D12Resource*         _pTexture,
            UINT                    _descriptorSize,
            UINT                    _textureIndex,
            UINT                    _mipUavBaseOffset,
            UINT                    _maxMipLevelsPerTexture
        ) const;

        void GenerateMipmaps(
            ID3D12GraphicsCommandList*                      _pCmdList,
            ID3D12Resource*                                 _pTexture,
            D3D12_GPU_DESCRIPTOR_HANDLE                     _srvGpuHandle,
            const std::vector<D3D12_GPU_DESCRIPTOR_HANDLE>& _uavGpuHandles,
            ID3D12PipelineState*                            _pMipGenPipelineState,
            ID3D12RootSignature*                            _pMipGenRootSignature
        ) const;

    private:

        std::vector<cGpuTexture> m_textures;
};