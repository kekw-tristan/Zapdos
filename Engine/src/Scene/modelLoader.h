#pragma once

#include <DirectXMath.h>
#include <string>
#include <unordered_map>
#include <cstdint>

namespace tinygltf
{
    class Model;
    class Image;
}

using namespace DirectX;

struct cCpuTexture;
struct sMaterial;
struct sMeshData;
struct sModel;

class cModelLoader
{
public:
    static void LoadGLTFModel(std::string& _rFilePath, sModel& _rOutModel);

private:
    static void ProcessNode(tinygltf::Model& _rModel, int _nodeIndex, XMMATRIX _parentWorldMatrix, sModel& _rOutModel);
    static void ExtractPrimitives(tinygltf::Model& model, int meshIndex, sModel& _rOutModel);
    static sMaterial ExtractMaterialFromGLTF(const tinygltf::Model& model, int materialIndex);
    static uint32_t GetOrCreateMaterialId(const tinygltf::Model& _rModel, int _materialIndex, sModel& _rOutModel);
    static void CreateTexturesFromGltf(const tinygltf::Model& _rModel, sModel& _rOutModel);

private:
    static std::unordered_map<int, uint32_t> s_gltfMaterialToEngineMaterial;
};