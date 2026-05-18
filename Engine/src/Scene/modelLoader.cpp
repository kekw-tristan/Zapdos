#include "modelLoader.h"

#include <stack>
#include <chrono>

#include "model.h"

#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#define TINYGLTF_IMPLEMENTATION
#include "tiny_gltf.h"

std::unordered_map<int, uint32_t> cModelLoader::s_gltfMaterialToEngineMaterial{};

// --------------------------------------------------------------------------------------------------------------------------

void cModelLoader::LoadGLTFModel(std::string& _rFilePath, sModel& _rOutModel)
{
	using Clock = std::chrono::high_resolution_clock;

	auto totalStart = Clock::now();

	_rOutModel.meshes.clear(); 
	_rOutModel.materials.clear();
	_rOutModel.worldMatrices.clear(); 
	_rOutModel.cpuTextures.clear();

	tinygltf::Model		model;
	tinygltf::TinyGLTF	loader;
	std::string			err, warn;

	auto parseStart = Clock::now();

	bool ret = false;

	try
	{
		if (_rFilePath.size() >= 4 &&
			_rFilePath.substr(_rFilePath.size() - 4) == ".glb")
		{
			ret = loader.LoadBinaryFromFile(
				&model,
				&err,
				&warn,
				_rFilePath);
		}
		else
		{
			ret = loader.LoadASCIIFromFile(
				&model,
				&err,
				&warn,
				_rFilePath);
		}
	}
	catch (const std::exception& rException)
	{
		std::cout
			<< "GLTF EXCEPTION: "
			<< rException.what()
			<< std::endl;

		return;
	}

	auto parseEnd =
		Clock::now();

	std::cout
		<< "GLTF parse/load: "
		<< std::chrono::duration<double>(
			parseEnd - parseStart).count()
		<< " seconds\n";

	_rOutModel.meshes.reserve(model.meshes.size());
	_rOutModel.materials.reserve(model.materials.size());
	_rOutModel.worldMatrices.reserve(model.nodes.size());
	_rOutModel.cpuTextures.reserve(model.textures.size());

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

	auto materialStart = Clock::now();

	for (size_t i = 0; i < model.materials.size(); ++i)
	{
		_rOutModel.materials.push_back(
			ExtractMaterialFromGLTF(model, (int)i));
	}

	auto materialEnd =
		Clock::now();

	std::cout
		<< "Material extraction: "
		<< std::chrono::duration<double>(
			materialEnd - materialStart).count()
		<< " seconds\n";
	
	auto meshStart =
		Clock::now();
	ProcessSceneIterative(model, scene, _rOutModel);

	auto meshEnd =
		Clock::now();

	std::cout
		<< "Mesh extraction: "
		<< std::chrono::duration<double>(
			meshEnd - meshStart).count()
		<< " seconds\n";

	auto textureStart =
		Clock::now();
	CreateTexturesFromGltf(model, _rOutModel);

	auto textureEnd =
		Clock::now();

	std::cout
		<< "Texture processing: "
		<< std::chrono::duration<double>(
			textureEnd - textureStart).count()
		<< " seconds\n";
}

// --------------------------------------------------------------------------------------------------------------------------

void cModelLoader::ProcessSceneIterative(tinygltf::Model& _rModel, const tinygltf::Scene& _rScene, sModel& _rOutModel)
{
	std::vector<sNodeJob> stack;
	stack.reserve(_rModel.nodes.size());

	for (int root : _rScene.nodes)
	{
		stack.push_back({ root, XMMatrixIdentity() });
	}

	while (!stack.empty())
	{
		sNodeJob job = stack.back();
		stack.pop_back();

		const tinygltf::Node& node = _rModel.nodes[job.nodeIndex];

		XMMATRIX localRH = XMMatrixIdentity();

		if (node.matrix.size() == 16)
		{
			localRH = XMMatrixSet(
				(float)node.matrix[0], (float)node.matrix[1],
				(float)node.matrix[2], (float)node.matrix[3],

				(float)node.matrix[4], (float)node.matrix[5],
				(float)node.matrix[6], (float)node.matrix[7],

				(float)node.matrix[8], (float)node.matrix[9],
				(float)node.matrix[10], (float)node.matrix[11],

				(float)node.matrix[12], (float)node.matrix[13],
				(float)node.matrix[14], (float)node.matrix[15]
			);
		}
		else
		{
			XMVECTOR t = XMVectorSet(
				node.translation.size() > 0 ? (float)node.translation[0] : 0.f,
				node.translation.size() > 1 ? (float)node.translation[1] : 0.f,
				node.translation.size() > 2 ? (float)node.translation[2] : 0.f,
				1.f);

			XMVECTOR r = XMQuaternionIdentity();

			if (node.rotation.size() == 4)
			{
				r = XMVectorSet(
					(float)node.rotation[0],
					(float)node.rotation[1],
					(float)node.rotation[2],
					(float)node.rotation[3]);
			}

			XMVECTOR s = XMVectorSet(
				node.scale.size() > 0 ? (float)node.scale[0] : 1.f,
				node.scale.size() > 1 ? (float)node.scale[1] : 1.f,
				node.scale.size() > 2 ? (float)node.scale[2] : 1.f,
				0.f);

			localRH = XMMatrixAffineTransformation(
				s,
				XMVectorZero(),
				r,
				t);
		}

		static const XMMATRIX flipX =
			XMMatrixScaling(-1.f, 1.f, 1.f);

		XMMATRIX localLH =
			flipX * localRH * flipX;

		XMMATRIX world =
			localLH * job.parentMatrix;

		if (node.mesh >= 0)
		{
			size_t oldCount = _rOutModel.meshes.size();

			ExtractPrimitives(_rModel, node.mesh, _rOutModel);

			size_t newCount = _rOutModel.meshes.size();

			_rOutModel.worldMatrices.reserve(newCount);

			for (size_t i = oldCount; i < newCount; ++i)
			{
				_rOutModel.worldMatrices.push_back(world);
			}
		}

		for (int child : node.children)
		{
			stack.push_back({ child, world });
		}
	}
}

// ----------------------------------------------------------------------------------	----------------------------------------

void cModelLoader::ExtractPrimitives(tinygltf::Model& model, int meshIndex, sModel& _rOutModel)
{
    const tinygltf::Mesh& mesh = model.meshes[meshIndex];

    std::vector<sMeshData>& rMeshes =
        _rOutModel.meshes;

    for (const auto& primitive : mesh.primitives)
    {
        sMeshData meshData{};

        // =========================================================
        // ATTRIBUTE LOOKUPS
        // =========================================================

        const auto& rAttributes =
            primitive.attributes;

        auto posIt  = rAttributes.find("POSITION");
        auto normIt = rAttributes.find("NORMAL");
        auto uvIt   = rAttributes.find("TEXCOORD_0");

        // =========================================================
        // POINTERS
        // =========================================================

        const float* pPositions = nullptr;
        const float* pNormals   = nullptr;
        const float* pTexcoords = nullptr;

        size_t vertexCount = 0;

        // =========================================================
        // POSITION POINTER
        // =========================================================

        if (posIt != rAttributes.end())
        {
            const tinygltf::Accessor& accessor =
                model.accessors[posIt->second];

            const tinygltf::BufferView& view =
                model.bufferViews[accessor.bufferView];

            const tinygltf::Buffer& buffer =
                model.buffers[view.buffer];

            pPositions =
                reinterpret_cast<const float*>(
                    buffer.data.data() +
                    view.byteOffset +
                    accessor.byteOffset);

            vertexCount =
                static_cast<size_t>(accessor.count);
        }

        // =========================================================
        // NORMAL POINTER
        // =========================================================

        if (normIt != rAttributes.end())
        {
            const tinygltf::Accessor& accessor =
                model.accessors[normIt->second];

            const tinygltf::BufferView& view =
                model.bufferViews[accessor.bufferView];

            const tinygltf::Buffer& buffer =
                model.buffers[view.buffer];

            pNormals =
                reinterpret_cast<const float*>(
                    buffer.data.data() +
                    view.byteOffset +
                    accessor.byteOffset);
        }

        // =========================================================
        // TEXCOORD POINTER
        // =========================================================

        if (uvIt != rAttributes.end())
        {
            const tinygltf::Accessor& accessor =
                model.accessors[uvIt->second];

            const tinygltf::BufferView& view =
                model.bufferViews[accessor.bufferView];

            const tinygltf::Buffer& buffer =
                model.buffers[view.buffer];

            pTexcoords =
                reinterpret_cast<const float*>(
                    buffer.data.data() +
                    view.byteOffset +
                    accessor.byteOffset);
        }

        // =========================================================
        // SINGLE VERTEX LOOP
        // =========================================================

        meshData.vertices.resize(vertexCount);

        for (size_t i = 0; i < vertexCount; ++i)
        {
            sVertex& rVertex =
                meshData.vertices[i];

            // -----------------------------
            // POSITION
            // -----------------------------

            if (pPositions)
            {
                rVertex.position.x =
                    -pPositions[i * 3 + 0];

                rVertex.position.y =
                     pPositions[i * 3 + 1];

                rVertex.position.z =
                     pPositions[i * 3 + 2];
            }

            // -----------------------------
            // NORMAL
            // -----------------------------

            if (pNormals)
            {
                rVertex.normal.x =
                    -pNormals[i * 3 + 0];

                rVertex.normal.y =
                     pNormals[i * 3 + 1];

                rVertex.normal.z =
                     pNormals[i * 3 + 2];
            }

            // -----------------------------
            // TEXCOORD
            // -----------------------------

            if (pTexcoords)
            {
                rVertex.texC.x =
                    pTexcoords[i * 2 + 0];

                rVertex.texC.y =
                    pTexcoords[i * 2 + 1];
            }
        }

        // =========================================================
        // INDICES
        // =========================================================

		if (primitive.indices >= 0)
		{
			const tinygltf::Accessor& accessor =
				model.accessors[primitive.indices];

			const tinygltf::BufferView& view =
				model.bufferViews[accessor.bufferView];

			const tinygltf::Buffer& buffer =
				model.buffers[view.buffer];

			const unsigned char* pData =
				buffer.data.data() +
				view.byteOffset +
				accessor.byteOffset;

			const size_t indexCount =
				static_cast<size_t>(accessor.count);

			meshData.indices32.resize(indexCount);

			// -----------------------------------------------------
			// UINT16
			// -----------------------------------------------------
			if (accessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT)
			{
				const uint16_t* pIndices =
					reinterpret_cast<const uint16_t*>(pData);

				for (size_t i = 0; i < indexCount; i += 3)
				{
					meshData.indices32[i + 0] =
						static_cast<uint32_t>(pIndices[i + 0]);

					meshData.indices32[i + 1] =
						static_cast<uint32_t>(pIndices[i + 2]);

					meshData.indices32[i + 2] =
						static_cast<uint32_t>(pIndices[i + 1]);
				}
			}

			// -----------------------------------------------------
			// UINT32
			// -----------------------------------------------------
			else if (accessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT)
			{
				const uint32_t* pIndices =
					reinterpret_cast<const uint32_t*>(pData);

				for (size_t i = 0; i < indexCount; i += 3)
				{
					meshData.indices32[i + 0] = pIndices[i + 0];
					meshData.indices32[i + 1] = pIndices[i + 2];
					meshData.indices32[i + 2] = pIndices[i + 1];
				}
			}
			else
			{
				throw std::runtime_error("Unsupported glTF index component type.");
			}

			// =========================================================
			// MATERIAL
			// =========================================================

			meshData.materialId =
				primitive.material;

			rMeshes.push_back(std::move(meshData));
		}
    }
}

// --------------------------------------------------------------------------------------------------------------------------

sMaterial cModelLoader::ExtractMaterialFromGLTF(const tinygltf::Model& model, int materialIndex)
{
	sMaterial mat{};

	// glTF PBR Defaults
	mat.albedo = XMFLOAT3(1.0f, 1.0f, 1.0f);
	mat.alpha = 1.0f;

	mat.metallic = 1.0f;
	mat.roughness = 1.0f;
	mat.ao = 1.0f;

	mat.emissive = XMFLOAT3(0.0f, 0.0f, 0.0f);

	mat.baseColorIndex = -1;
	mat.metallicRoughnessIndex = -1;
	mat.normalIndex = -1;
	mat.occlusionIndex = -1;
	mat.emissiveIndex = -1;

	mat.normalScale = 1.0f;
	mat.occlusionStrength = 1.0f;

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

	const tinygltf::PbrMetallicRoughness& pbr = gltfMat.pbrMetallicRoughness;

	// ------------------------------------------------------------
	// Base Color Factor
	// ------------------------------------------------------------
	if (pbr.baseColorFactor.size() == 4)
	{
		mat.albedo = XMFLOAT3(
			static_cast<float>(pbr.baseColorFactor[0]),
			static_cast<float>(pbr.baseColorFactor[1]),
			static_cast<float>(pbr.baseColorFactor[2])
		);

		mat.alpha = static_cast<float>(pbr.baseColorFactor[3]);
	}

	// ------------------------------------------------------------
	// Base Color Texture
	// RGBA, meistens sRGB
	// ------------------------------------------------------------
	if (pbr.baseColorTexture.index >= 0)
	{
		mat.baseColorIndex = ResolveTextureToImageIndex(pbr.baseColorTexture.index);
	}

	// ------------------------------------------------------------
	// Metallic / Roughness Factors
	// ------------------------------------------------------------
	mat.metallic = static_cast<float>(pbr.metallicFactor);
	mat.roughness = static_cast<float>(pbr.roughnessFactor);

	// ------------------------------------------------------------
	// Metallic-Roughness Texture
	//
	// glTF Packing:
	// R = unused
	// G = roughness
	// B = metallic
	// A = unused
	// ------------------------------------------------------------
	if (pbr.metallicRoughnessTexture.index >= 0)
	{
		mat.metallicRoughnessIndex =
			ResolveTextureToImageIndex(pbr.metallicRoughnessTexture.index);
	}

	// ------------------------------------------------------------
	// Normal Texture
	// Tangent-Space Normal Map
	// ------------------------------------------------------------
	if (gltfMat.normalTexture.index >= 0)
	{
		mat.normalIndex = ResolveTextureToImageIndex(gltfMat.normalTexture.index);
		mat.normalScale = static_cast<float>(gltfMat.normalTexture.scale);
	}

	// ------------------------------------------------------------
	// Occlusion Texture
	//
	// glTF Packing:
	// R = ambient occlusion
	// ------------------------------------------------------------
	if (gltfMat.occlusionTexture.index >= 0)
	{
		mat.occlusionIndex = ResolveTextureToImageIndex(gltfMat.occlusionTexture.index);
		mat.occlusionStrength = static_cast<float>(gltfMat.occlusionTexture.strength);
	}

	// ------------------------------------------------------------
	// Emissive Factor
	// ------------------------------------------------------------
	if (gltfMat.emissiveFactor.size() == 3)
	{
		mat.emissive = XMFLOAT3(
			static_cast<float>(gltfMat.emissiveFactor[0]),
			static_cast<float>(gltfMat.emissiveFactor[1]),
			static_cast<float>(gltfMat.emissiveFactor[2])
		);
	}

	// ------------------------------------------------------------
	// Emissive Texture
	// RGB, meistens sRGB
	// ------------------------------------------------------------
	if (gltfMat.emissiveTexture.index >= 0)
	{
		mat.emissiveIndex = ResolveTextureToImageIndex(gltfMat.emissiveTexture.index);
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

void cModelLoader::CreateTexturesFromGltf(tinygltf::Model& _rModel, sModel& _rOutModel)
{
	std::vector<cCpuTexture>& rTextures = _rOutModel.cpuTextures;

	for (auto& image : _rModel.images)
	{
		int width = image.width;
		int height = image.height;


		cCpuTexture texture(width, height, std::move(image.image));
		rTextures.emplace_back(std::move(texture));
	}

	std::cout << "number of cpu Textures: " << rTextures.size() << std::endl;
}

// --------------------------------------------------------------------------------------------------------------------------