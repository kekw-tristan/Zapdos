#include "modelLoader.h"

#include "model.h"

#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#define TINYGLTF_IMPLEMENTATION
#include "tiny_gltf.h"

std::unordered_map<int, uint32_t> cModelLoader::s_gltfMaterialToEngineMaterial{};

// --------------------------------------------------------------------------------------------------------------------------

void cModelLoader::LoadGLTFModel(std::string& _rFilePath, sModel& _rOutModel)
{
	_rOutModel.meshes.clear(); 
	_rOutModel.materials.clear();
	_rOutModel.worldMatrices.clear(); 
	_rOutModel.cpuTextures.clear();

	tinygltf::Model		model;
	tinygltf::TinyGLTF	loader;
	std::string			err, warn;

	bool ret = false;
	if (_rFilePath.size() >= 4 && _rFilePath.substr(_rFilePath.size() - 4) == ".glb")
		ret = loader.LoadBinaryFromFile(&model, &err, &warn, _rFilePath);
	else
		ret = loader.LoadASCIIFromFile(&model, &err, &warn, _rFilePath);

	std::cout << "\n========== GLTF DEBUG ==========\n";
	std::cout << "file: " << _rFilePath << "\n";
	std::cout << "ret: " << ret << "\n";
	std::cout << "warn: " << (warn.empty() ? "<none>" : warn) << "\n";
	std::cout << "err : " << (err.empty() ? "<none>" : err) << "\n";

	std::cout << "model.scenes:      " << model.scenes.size() << "\n";
	std::cout << "model.defaultScene:" << model.defaultScene << "\n";
	std::cout << "model.nodes:       " << model.nodes.size() << "\n";
	std::cout << "model.meshes:      " << model.meshes.size() << "\n";
	std::cout << "model.materials:   " << model.materials.size() << "\n";
	std::cout << "model.textures:    " << model.textures.size() << "\n";
	std::cout << "model.images:      " << model.images.size() << "\n";
	std::cout << "model.buffers:     " << model.buffers.size() << "\n";
	std::cout << "model.bufferViews: " << model.bufferViews.size() << "\n";
	std::cout << "model.accessors:   " << model.accessors.size() << "\n";

	std::cout << "extensionsUsed: ";
	for (const auto& e : model.extensionsUsed) std::cout << e << " ";
	std::cout << "\n";

	std::cout << "extensionsRequired: ";
	for (const auto& e : model.extensionsRequired) std::cout << e << " ";
	std::cout << "\n";

	if (!ret)
	{
		std::cerr << "Failed to load GLTF: " << _rFilePath << std::endl;
		std::cout << "================================\n";
		return;
	}

	if (model.scenes.empty())
	{
		std::cerr << "GLTF Error: No scenes found.\n";
		std::cout << "================================\n";
		return;
	}

	int sceneIndex = (model.defaultScene >= 0) ? model.defaultScene : 0;
	std::cout << "chosen scene index: " << sceneIndex << "\n";

	const tinygltf::Scene& scene = model.scenes[sceneIndex];
	std::cout << "scene.nodes.size(): " << scene.nodes.size() << "\n";

	for (size_t i = 0; i < scene.nodes.size(); ++i)
	{
		int nodeIndex = scene.nodes[i];
		const auto& node = model.nodes[nodeIndex];
	}

	for (int rootNodeIndex : scene.nodes)
	{
		XMMATRIX identity = XMMatrixIdentity();
		ProcessNode(model, rootNodeIndex, identity, _rOutModel);
	}

	CreateTexturesFromGltf(model, _rOutModel);
}

// --------------------------------------------------------------------------------------------------------------------------

void cModelLoader::ProcessNode(tinygltf::Model& _rModel, int _nodeIndex, XMMATRIX _parentWorldMatrix, sModel& _rOutModel)
{
	const tinygltf::Node& node = _rModel.nodes[_nodeIndex];

	std::vector<sMeshData>&	rMeshes			= _rOutModel.meshes;
	std::vector<XMMATRIX>&	rWorldMatrices	= _rOutModel.worldMatrices;

	// -------------------------------
	// Build local transform (RH)
	// -------------------------------
	XMMATRIX localRH = XMMatrixIdentity();

	if (node.matrix.size() == 16)
	{
		localRH = XMMatrixSet(
			(float)node.matrix[0], (float)node.matrix[1], (float)node.matrix[2], (float)node.matrix[3],
			(float)node.matrix[4], (float)node.matrix[5], (float)node.matrix[6], (float)node.matrix[7],
			(float)node.matrix[8], (float)node.matrix[9], (float)node.matrix[10], (float)node.matrix[11],
			(float)node.matrix[12], (float)node.matrix[13], (float)node.matrix[14], (float)node.matrix[15]
		);
	}
	else
	{
		XMVECTOR translation = XMVectorSet(
			node.translation.size() > 0 ? (float)node.translation[0] : 0.f,
			node.translation.size() > 1 ? (float)node.translation[1] : 0.f,
			node.translation.size() > 2 ? (float)node.translation[2] : 0.f,
			1.f);

		XMVECTOR rotation = XMQuaternionIdentity();
		if (node.rotation.size() == 4)
		{
			rotation = XMVectorSet(
				(float)node.rotation[0],
				(float)node.rotation[1],
				(float)node.rotation[2],
				(float)node.rotation[3]);
		}

		XMVECTOR scale = XMVectorSet(
			node.scale.size() > 0 ? (float)node.scale[0] : 1.f,
			node.scale.size() > 1 ? (float)node.scale[1] : 1.f,
			node.scale.size() > 2 ? (float)node.scale[2] : 1.f,
			0.f);

		localRH = XMMatrixAffineTransformation(
			scale,
			XMVectorZero(),
			rotation,
			translation);
	}

	const XMMATRIX flipX = XMMatrixScaling(-1.0f, 1.0f, 1.0f);
	XMMATRIX localLH = flipX * localRH * flipX;

	// -------------------------------
	// Combine with parent (LH)
	// -------------------------------
	XMMATRIX worldMatrix = localLH * _parentWorldMatrix;

	// -------------------------------
	// Extract mesh
	// -------------------------------
	if (node.mesh >= 0)
	{
		const size_t oldMeshCount = rMeshes.size();

		ExtractPrimitives(_rModel, node.mesh, _rOutModel);

		const size_t newMeshCount = rMeshes.size();

		for (size_t i = oldMeshCount; i < newMeshCount; ++i)
		{
			rWorldMatrices.push_back(worldMatrix);
		}
	}

	// -------------------------------
	// Children
	// -------------------------------
	for (int childIndex : node.children)
	{
		ProcessNode(
			_rModel,
			childIndex,
			worldMatrix,
			_rOutModel);
	}
}

// --------------------------------------------------------------------------------------------------------------------------

void cModelLoader::ExtractPrimitives(tinygltf::Model& model, int meshIndex, sModel& _rOutModel)
{
	const tinygltf::Mesh& mesh = model.meshes[meshIndex];

	std::vector<sMeshData>& rMeshes = _rOutModel.meshes;

	for (const auto& primitive : mesh.primitives)
	{
		sMeshData meshData = {};

		// ===============================
		// Extract indices 
		// ===============================
		if (primitive.indices >= 0)
		{
			const tinygltf::Accessor& accessor = model.accessors[primitive.indices];
			const tinygltf::BufferView& view = model.bufferViews[accessor.bufferView];
			const tinygltf::Buffer& buffer = model.buffers[view.buffer];

			const unsigned char* pData =
				buffer.data.data() + view.byteOffset + accessor.byteOffset;

			const size_t indexCount = static_cast<size_t>(accessor.count);
			const uint32_t vertexOffset = static_cast<uint32_t>(meshData.vertices.size());

			// glTF primitives are TRIANGLES
			if (accessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT)
			{
				const uint16_t* indices = reinterpret_cast<const uint16_t*>(pData);
				meshData.indices16.reserve(meshData.indices16.size() + indexCount);

				for (size_t i = 0; i < indexCount; i += 3)
				{
					meshData.indices16.push_back(indices[i + 0] + vertexOffset);
					meshData.indices16.push_back(indices[i + 2] + vertexOffset); // swap
					meshData.indices16.push_back(indices[i + 1] + vertexOffset); // swap
				}
			}
			else if (accessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT)
			{
				const uint32_t* indices = reinterpret_cast<const uint32_t*>(pData);
				meshData.indices32.reserve(meshData.indices32.size() + indexCount);

				for (size_t i = 0; i < indexCount; i += 3)
				{
					meshData.indices32.push_back(indices[i + 0] + vertexOffset);
					meshData.indices32.push_back(indices[i + 2] + vertexOffset); // swap
					meshData.indices32.push_back(indices[i + 1] + vertexOffset); // swap
				}
			}
			else
			{
				std::cout << "Index type not supported!\n";
			}
		}

		// ===============================
		// Extract positions (RH -> LH)
		// ===============================
		auto posIt = primitive.attributes.find("POSITION");
		if (posIt != primitive.attributes.end())
		{
			const tinygltf::Accessor& accessor = model.accessors[posIt->second];
			const tinygltf::BufferView& view = model.bufferViews[accessor.bufferView];
			const tinygltf::Buffer& buffer = model.buffers[view.buffer];

			const unsigned char* pData =
				buffer.data.data() + view.byteOffset + accessor.byteOffset;

			const size_t vertexCount = static_cast<size_t>(accessor.count);
			const size_t baseVertex = meshData.vertices.size();

			meshData.vertices.resize(baseVertex + vertexCount);

			const float* positions = reinterpret_cast<const float*>(pData);
			for (size_t i = 0; i < vertexCount; ++i)
			{
				meshData.vertices[baseVertex + i].position = XMFLOAT3(
					-positions[i * 3 + 0], // X spiegeln
					positions[i * 3 + 1],
					positions[i * 3 + 2]
				);
			}
		}

		// ===============================
		// Extract normals (RH -> LH)
		// ===============================
		auto normIt = primitive.attributes.find("NORMAL");
		if (normIt != primitive.attributes.end())
		{
			const tinygltf::Accessor& accessor = model.accessors[normIt->second];
			const tinygltf::BufferView& view = model.bufferViews[accessor.bufferView];
			const tinygltf::Buffer& buffer = model.buffers[view.buffer];

			const unsigned char* pData =
				buffer.data.data() + view.byteOffset + accessor.byteOffset;

			const size_t count = static_cast<size_t>(accessor.count);
			const size_t startIndex = meshData.vertices.size() - count;

			const float* normals = reinterpret_cast<const float*>(pData);
			for (size_t i = 0; i < count; ++i)
			{
				meshData.vertices[startIndex + i].normal = XMFLOAT3(
					-normals[i * 3 + 0], // X spiegeln
					normals[i * 3 + 1],
					normals[i * 3 + 2]
				);
			}
		}

		// ===============================
		// Extract texcoords
		// ===============================
		auto uvIt = primitive.attributes.find("TEXCOORD_0");
		if (uvIt != primitive.attributes.end())
		{
			const tinygltf::Accessor& accessor = model.accessors[uvIt->second];
			const tinygltf::BufferView& view = model.bufferViews[accessor.bufferView];
			const tinygltf::Buffer& buffer = model.buffers[view.buffer];

			const unsigned char* pData =
				buffer.data.data() + view.byteOffset + accessor.byteOffset;

			const size_t count = static_cast<size_t>(accessor.count);
			const size_t startIndex = meshData.vertices.size() - count;

			const float* texcoords = reinterpret_cast<const float*>(pData);
			for (size_t i = 0; i < count; ++i)
			{
				meshData.vertices[startIndex + i].texC = XMFLOAT2(
					texcoords[i * 2 + 0],
					texcoords[i * 2 + 1]
				);
			}
		}

		// ===============================
		// Extract material
		// ===============================
		meshData.materialId = GetOrCreateMaterialId(model, primitive.material, _rOutModel);
		rMeshes.push_back(std::move(meshData));
	}
}

// --------------------------------------------------------------------------------------------------------------------------

sMaterial cModelLoader::ExtractMaterialFromGLTF(const tinygltf::Model& model, int materialIndex)
{
	sMaterial mat{};

	mat.albedo = XMFLOAT3(1.0f, 1.0f, 1.0f);
	mat.alpha = 1.0f;
	mat.metallic = 0.0f;
	mat.roughness = 1.0f;
	mat.ao = 1.0f;
	mat.emissive = XMFLOAT3(0.0f, 0.0f, 0.0f);
	mat.baseColorIndex = -1;

	if (materialIndex < 0 || materialIndex >= static_cast<int>(model.materials.size()))
		return mat;

	const tinygltf::Material& gltfMat = model.materials[materialIndex];

	auto ResolveTextureToImageIndex = [&](int gltfTextureIndex) -> int
		{
			if (gltfTextureIndex < 0 || gltfTextureIndex >= static_cast<int>(model.textures.size()))
				return -1;

			const int imageIndex = model.textures[gltfTextureIndex].source;

			if (imageIndex < 0 || imageIndex >= static_cast<int>(model.images.size()))
				return -1;

			return imageIndex;
		};

	// 1) Normaler glTF metallic-roughness Pfad
	const auto& pbr = gltfMat.pbrMetallicRoughness;

	if (pbr.baseColorFactor.size() == 4)
	{
		mat.albedo = XMFLOAT3(
			static_cast<float>(pbr.baseColorFactor[0]),
			static_cast<float>(pbr.baseColorFactor[1]),
			static_cast<float>(pbr.baseColorFactor[2]));
		mat.alpha = static_cast<float>(pbr.baseColorFactor[3]);
	}

	if (pbr.baseColorTexture.index >= 0)
	{
		mat.baseColorIndex = ResolveTextureToImageIndex(pbr.baseColorTexture.index);
	}

	mat.metallic = static_cast<float>(pbr.metallicFactor);
	mat.roughness = static_cast<float>(pbr.roughnessFactor);

	// 2) Fallback / override für KHR_materials_pbrSpecularGlossiness
	auto extIt = gltfMat.extensions.find("KHR_materials_pbrSpecularGlossiness");
	if (extIt != gltfMat.extensions.end())
	{
		const tinygltf::Value& ext = extIt->second;

		// diffuseFactor
		if (ext.Has("diffuseFactor"))
		{
			const tinygltf::Value& diffuseFactor = ext.Get("diffuseFactor");
			if (diffuseFactor.IsArray() && diffuseFactor.ArrayLen() == 4)
			{
				mat.albedo = XMFLOAT3(
					static_cast<float>(diffuseFactor.Get(0).GetNumberAsDouble()),
					static_cast<float>(diffuseFactor.Get(1).GetNumberAsDouble()),
					static_cast<float>(diffuseFactor.Get(2).GetNumberAsDouble()));
				mat.alpha = static_cast<float>(diffuseFactor.Get(3).GetNumberAsDouble());
			}
		}

		// diffuseTexture -> das ist für dich die BaseColor Texture
		if (ext.Has("diffuseTexture"))
		{
			const tinygltf::Value& diffuseTexture = ext.Get("diffuseTexture");
			if (diffuseTexture.IsObject() && diffuseTexture.Has("index"))
			{
				int gltfTextureIndex = diffuseTexture.Get("index").GetNumberAsInt();
				mat.baseColorIndex = ResolveTextureToImageIndex(gltfTextureIndex);
			}
		}

		// glossinessFactor -> roughness
		if (ext.Has("glossinessFactor"))
		{
			const float glossiness = static_cast<float>(ext.Get("glossinessFactor").GetNumberAsDouble());
			mat.roughness = 1.0f - glossiness;
		}

		// spec-gloss ist nicht metallic-roughness.
		// Für deinen aktuellen Shader ist 0 als Näherung sinnvoller als 1.
		mat.metallic = 0.0f;
	}

	// Emissive
	if (gltfMat.emissiveFactor.size() == 3)
	{
		mat.emissive = XMFLOAT3(
			static_cast<float>(gltfMat.emissiveFactor[0]),
			static_cast<float>(gltfMat.emissiveFactor[1]),
			static_cast<float>(gltfMat.emissiveFactor[2]));
	}
	return mat;
}

// --------------------------------------------------------------------------------------------------------------------------

uint32_t cModelLoader::GetOrCreateMaterialId(const tinygltf::Model& _rModel, int _materialIndex, sModel& _rOutModel)
{
	std::vector<sMaterial>& rMaterials = _rOutModel.materials;

	if (_materialIndex < 0)
	{
		auto iterator = s_gltfMaterialToEngineMaterial.find(-1);
		if (iterator != s_gltfMaterialToEngineMaterial.end())
			return iterator->second;

		sMaterial defaultMat{};
		const uint32_t newId = static_cast<uint32_t>(rMaterials.size());
		rMaterials.push_back(std::move(defaultMat));
		s_gltfMaterialToEngineMaterial[-1] = newId;
		return newId;
	}

	auto iterator = s_gltfMaterialToEngineMaterial.find(_materialIndex);
	if (iterator != s_gltfMaterialToEngineMaterial.end())
		return iterator->second;

	sMaterial newMaterial = cModelLoader::ExtractMaterialFromGLTF(_rModel, _materialIndex);

	const uint32_t newId = static_cast<uint32_t>(rMaterials.size());
	rMaterials.push_back(std::move(newMaterial));
	s_gltfMaterialToEngineMaterial[_materialIndex] = newId;

	return newId;
}


// --------------------------------------------------------------------------------------------------------------------------

void cModelLoader::CreateTexturesFromGltf(const tinygltf::Model& _rModel, sModel& _rOutModel)
{
	std::vector<cCpuTexture>& rTextures = _rOutModel.cpuTextures;

	for (auto image : _rModel.images)
	{
		int width = image.width;
		int height = image.height;


		cCpuTexture texture(width, height, std::move(image.image));
		rTextures.emplace_back(std::move(texture));
	}

	std::cout << "number of cpu Textures: " << rTextures.size() << std::endl;
}

// --------------------------------------------------------------------------------------------------------------------------