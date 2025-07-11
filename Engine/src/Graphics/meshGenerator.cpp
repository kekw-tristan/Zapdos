#include "meshGenerator.h"

#include <iostream>

#include <array>
#include <unordered_map>

#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#define TINYGLTF_IMPLEMENTATION
#include "tiny_gltf.h"

// --------------------------------------------------------------------------------------------------------------------------
cMeshGenerator::sMeshData cMeshGenerator::CreateCylinder(float _bottomRadius, float _topRadius, float _height, uint32 _sliceCount, uint32 _stackCount)
{
	sMeshData meshData;

	float stackHeight = _height / _stackCount;
	float radiusStep = (_topRadius - _bottomRadius) / _stackCount;
	uint32 ringCount = _stackCount + 1;

	// Compute vertices
	for (uint32 stackIndex = 0; stackIndex < ringCount; stackIndex++)
	{
		float y = -0.5f * _height + stackIndex * stackHeight;
		float r = _bottomRadius + stackIndex * radiusStep;

		float dTheta = 2.f * XM_PI / _sliceCount;
		for (uint32 sliceIndex = 0; sliceIndex <= _sliceCount; sliceIndex++)
		{
			sVertex vertex;

			float c = cosf(sliceIndex * dTheta);
			float s = sinf(sliceIndex * dTheta);

			vertex.position = XMFLOAT3(r * c, y, r * s);

			vertex.texC.x = static_cast<float>(sliceIndex) / _sliceCount;
			vertex.texC.y = 1.f - static_cast<float>(stackIndex) / _stackCount;

			vertex.tangentU = XMFLOAT3(-s, 0.f, c);

			float dr = _bottomRadius - _topRadius;
			XMFLOAT3 bitangent(dr * c, -_height, dr * s);

			XMVECTOR T = XMLoadFloat3(&vertex.tangentU);
			XMVECTOR B = XMLoadFloat3(&bitangent);
			XMVECTOR N = XMVector3Normalize(XMVector3Cross(T, B));
			XMStoreFloat3(&vertex.normal, N);

			meshData.vertices.push_back(vertex);
		}
	}

	uint32 ringVertexCount = _sliceCount + 1;

	// Side indices (corrected to CW)
	for (uint32 stackIndex = 0; stackIndex < _stackCount; ++stackIndex)
	{
		for (uint32 sliceIndex = 0; sliceIndex < _sliceCount; ++sliceIndex)
		{
			uint32 current = stackIndex * ringVertexCount + sliceIndex;
			uint32 next = (stackIndex + 1) * ringVertexCount + sliceIndex;

			// Triangle 1 (CW)
			meshData.indices32.push_back(current);
			meshData.indices32.push_back(next);
			meshData.indices32.push_back(next + 1);

			// Triangle 2 (CW)
			meshData.indices32.push_back(current);
			meshData.indices32.push_back(next + 1);
			meshData.indices32.push_back(current + 1);
		}
	}

	BuildCylinderTopCap(_topRadius, _height, _sliceCount, meshData);
	BuildCylinderBottomCap(_bottomRadius, _height, _sliceCount, meshData);

	return meshData;
}

// --------------------------------------------------------------------------------------------------------------------------

void cMeshGenerator::BuildCylinderTopCap(float _topRadius, float _height, uint32 _sliceCount, sMeshData& meshData)
{
	uint32 baseIndex = static_cast<uint32>(meshData.vertices.size());
	float y = 0.5f * _height;
	float dTheta = 2.f * XM_PI / _sliceCount;

	for (uint32 sliceIndex = 0; sliceIndex <= _sliceCount; ++sliceIndex)
	{
		float x = _topRadius * cosf(sliceIndex * dTheta);
		float z = _topRadius * sinf(sliceIndex * dTheta);

		float u = x / (2.0f * _topRadius) + 0.5f;
		float v = z / (2.0f * _topRadius) + 0.5f;

		sVertex vertex;
		vertex.position = XMFLOAT3(x, y, z);
		vertex.normal = XMFLOAT3(0.f, 1.f, 0.f);
		vertex.tangentU = XMFLOAT3(1.f, 0.f, 0.f);
		vertex.texC = XMFLOAT2(u, v);

		meshData.vertices.push_back(vertex);
	}

	// Center vertex
	sVertex centerVertex;
	centerVertex.position = XMFLOAT3(0.f, y, 0.f);
	centerVertex.normal = XMFLOAT3(0.f, 1.f, 0.f);
	centerVertex.tangentU = XMFLOAT3(1.f, 0.f, 0.f);
	centerVertex.texC = XMFLOAT2(0.5f, 0.5f);

	meshData.vertices.push_back(centerVertex);

	uint32 centerIndex = static_cast<uint32>(meshData.vertices.size() - 1);

	// Indices (CW winding)
	for (uint32 sliceIndex = 0; sliceIndex < _sliceCount; ++sliceIndex)
	{
		meshData.indices32.push_back(centerIndex);
		meshData.indices32.push_back(baseIndex + sliceIndex + 1);
		meshData.indices32.push_back(baseIndex + sliceIndex);
	}
}

// --------------------------------------------------------------------------------------------------------------------------

void cMeshGenerator::BuildCylinderBottomCap(float _bottomRadius, float _height, uint32 _sliceCount, sMeshData& meshData)
{
	uint32 baseIndex = static_cast<uint32>(meshData.vertices.size());
	float y = -0.5f * _height;
	float dTheta = 2.f * XM_PI / _sliceCount;

	for (uint32 sliceIndex = 0; sliceIndex <= _sliceCount; ++sliceIndex)
	{
		float x = _bottomRadius * cosf(sliceIndex * dTheta);
		float z = _bottomRadius * sinf(sliceIndex * dTheta);

		float u = x / (2.0f * _bottomRadius) + 0.5f;
		float v = z / (2.0f * _bottomRadius) + 0.5f;

		sVertex vertex;
		vertex.position = XMFLOAT3(x, y, z);
		vertex.normal = XMFLOAT3(0.f, -1.f, 0.f);
		vertex.tangentU = XMFLOAT3(1.f, 0.f, 0.f);
		vertex.texC = XMFLOAT2(u, v);

		meshData.vertices.push_back(vertex);
	}

	// Center vertex
	sVertex centerVertex;
	centerVertex.position = XMFLOAT3(0.f, y, 0.f);
	centerVertex.normal = XMFLOAT3(0.f, -1.f, 0.f);
	centerVertex.tangentU = XMFLOAT3(1.f, 0.f, 0.f);
	centerVertex.texC = XMFLOAT2(0.5f, 0.5f);

	meshData.vertices.push_back(centerVertex);

	uint32 centerIndex = static_cast<uint32>(meshData.vertices.size() - 1);

	// Indices (CW winding)
	for (uint32 sliceIndex = 0; sliceIndex < _sliceCount; ++sliceIndex)
	{
		meshData.indices32.push_back(centerIndex);
		meshData.indices32.push_back(baseIndex + sliceIndex);
		meshData.indices32.push_back(baseIndex + sliceIndex + 1);
	}
}

// --------------------------------------------------------------------------------------------------------------------------

cMeshGenerator::sMeshData cMeshGenerator::CreateCube()
{
	sMeshData meshData;

	// Define positions for each face (quad: 2 triangles, 4 vertices)
	struct sFace
	{
		XMFLOAT3 a, b, c, d;
		XMFLOAT3 normal;
	};

	float half = 0.5f;

	std::vector<sFace> faces = {
		// Front face
		{{-half, -half, -half}, { half, -half, -half}, { half,  half, -half}, {-half,  half, -half}, {0, 0, -1}},
		// Back face
		{{ half, -half,  half}, {-half, -half,  half}, {-half,  half,  half}, { half,  half,  half}, {0, 0, 1}},
		// Left face
		{{-half, -half,  half}, {-half, -half, -half}, {-half,  half, -half}, {-half,  half,  half}, {-1, 0, 0}},
		// Right face
		{{ half, -half, -half}, { half, -half,  half}, { half,  half,  half}, { half,  half, -half}, {1, 0, 0}},
		// Top face
		{{-half,  half, -half}, { half,  half, -half}, { half,  half,  half}, {-half,  half,  half}, {0, 1, 0}},
		// Bottom face
		{{-half, -half,  half}, { half, -half,  half}, { half, -half, -half}, {-half, -half, -half}, {0, -1, 0}},
	};

	for (const auto& face : faces)
	{
		uint32_t start = static_cast<uint32_t>(meshData.vertices.size());

		// Basic UVs for a quad
		std::array<XMFLOAT2, 4> uvs = {
			XMFLOAT2(0, 1),
			XMFLOAT2(1, 1),
			XMFLOAT2(1, 0),
			XMFLOAT2(0, 0)
		};

		std::array<XMFLOAT3, 4> verts = { face.a, face.b, face.c, face.d };
		for (int i = 0; i < 4; ++i)
		{
			sVertex v;
			v.position = verts[i];
			v.normal = face.normal;
			v.tangentU = { 1, 0, 0 }; // placeholder
			v.texC = uvs[i];
			meshData.vertices.push_back(v);
		}

		meshData.indices32.push_back(start + 0);
		meshData.indices32.push_back(start + 2);
		meshData.indices32.push_back(start + 1);

		meshData.indices32.push_back(start + 0);
		meshData.indices32.push_back(start + 3);
		meshData.indices32.push_back(start + 2);
	}

	return meshData;
}

// --------------------------------------------------------------------------------------------------------------------------

cMeshGenerator::sMeshData cMeshGenerator::CreateSphere(float radius, uint32_t sliceCount, uint32_t stackCount)
{
	sMeshData meshData;

	// Top vertex (north pole)
	sVertex topVertex;
	topVertex.position = XMFLOAT3(0.0f, +radius, 0.0f);
	topVertex.normal = XMFLOAT3(0.0f, +1.0f, 0.0f);
	topVertex.tangentU = XMFLOAT3(1.0f, 0.0f, 0.0f);
	topVertex.texC = XMFLOAT2(0.0f, 0.0f);
	meshData.vertices.push_back(topVertex);

	float phiStep = XM_PI / stackCount;
	float thetaStep = 2.0f * XM_PI / sliceCount;

	// Compute vertices for each stack ring (excluding top and bottom poles)
	for (uint32_t i = 1; i <= stackCount - 1; ++i)
	{
		float phi = i * phiStep;

		for (uint32_t j = 0; j <= sliceCount; ++j)
		{
			float theta = j * thetaStep;

			sVertex vertex;

			// Position
			vertex.position.x = radius * sinf(phi) * cosf(theta);
			vertex.position.y = radius * cosf(phi);
			vertex.position.z = radius * sinf(phi) * sinf(theta);

			// Normal
			XMVECTOR p = XMLoadFloat3(&vertex.position);
			XMStoreFloat3(&vertex.normal, XMVector3Normalize(p));

			// Texture coordinates
			vertex.texC.x = theta / XM_2PI;
			vertex.texC.y = phi / XM_PI;

			// Tangent (pointing in the theta direction)
			vertex.tangentU = XMFLOAT3(-radius * sinf(phi) * sinf(theta), 0.0f, radius * sinf(phi) * cosf(theta));
			XMStoreFloat3(&vertex.tangentU, XMVector3Normalize(XMLoadFloat3(&vertex.tangentU)));

			meshData.vertices.push_back(vertex);
		}
	}

	// Bottom vertex (south pole)
	sVertex bottomVertex;
	bottomVertex.position = XMFLOAT3(0.0f, -radius, 0.0f);
	bottomVertex.normal = XMFLOAT3(0.0f, -1.0f, 0.0f);
	bottomVertex.tangentU = XMFLOAT3(1.0f, 0.0f, 0.0f);
	bottomVertex.texC = XMFLOAT2(0.0f, 1.0f);
	meshData.vertices.push_back(bottomVertex);

	// --- Indices ---

	// Top cap indices (triangle fan) - switched to CW
	for (uint32_t i = 1; i <= sliceCount; ++i)
	{
		meshData.indices32.push_back(i % sliceCount + 1);   // next vertex
		meshData.indices32.push_back(i);                    // current vertex
		meshData.indices32.push_back(0);                    // top vertex
	}

	// Body indices (quads split into two triangles) - CW
	uint32_t baseIndex = 1;
	uint32_t ringVertexCount = sliceCount + 1;

	for (uint32_t i = 0; i < stackCount - 2; ++i)
	{
		for (uint32_t j = 0; j < sliceCount; ++j)
		{
			// Triangle 1 (CW)
			meshData.indices32.push_back(baseIndex + (i + 1) * ringVertexCount + j + 1);
			meshData.indices32.push_back(baseIndex + (i + 1) * ringVertexCount + j);
			meshData.indices32.push_back(baseIndex + i * ringVertexCount + j);

			// Triangle 2 (CW)
			meshData.indices32.push_back(baseIndex + i * ringVertexCount + j + 1);
			meshData.indices32.push_back(baseIndex + (i + 1) * ringVertexCount + j + 1);
			meshData.indices32.push_back(baseIndex + i * ringVertexCount + j);
		}
	}

	// Bottom cap indices (triangle fan) - switched to CW
	uint32_t southPoleIndex = static_cast<uint32_t>(meshData.vertices.size()) - 1;
	baseIndex = southPoleIndex - ringVertexCount;

	for (uint32_t i = 0; i < sliceCount; ++i)
	{
		meshData.indices32.push_back(baseIndex + i);                          // current vertex
		meshData.indices32.push_back(baseIndex + (i + 1) % ringVertexCount);  // next vertex
		meshData.indices32.push_back(southPoleIndex);                        // bottom vertex
	}

	return meshData;
}

// --------------------------------------------------------------------------------------------------------------------------

void cMeshGenerator::LoadModelFromGLTF(std::string& _rFilePath, sMeshData& _rOutMeshData)
{
	tinygltf::Model model;
	tinygltf::TinyGLTF loader;
	std::string err, warn;

	bool ret;
	if (_rFilePath.substr(_rFilePath.size() - 4) == ".glb")
	{
		ret = loader.LoadBinaryFromFile(&model, &err, &warn, _rFilePath);
	}
	else 
	{
		ret = loader.LoadASCIIFromFile(&model, &err, &warn, _rFilePath);
	}

	if (!warn.empty()) 
	{
		std::cout << "GLTF Warning: " << warn << std::endl;
	}
	if (!err.empty()) 
	{
		std::cerr << "GLTF Error: " << err << std::endl;
	}
	if (!ret) 
	{
		std::cerr << "Failed to load GLTF: " << _rFilePath << std::endl;
		return;
	}
	
	std::cout << "number of meshes: " << model.meshes.size() << "\n";

	for (const auto& mesh : model.meshes)
	{
	
	
		for (const auto& primitive : mesh.primitives)
		{
			const tinygltf::Accessor&	accessor	= model.accessors[primitive.indices];
			const tinygltf::BufferView& bufferView	= model.bufferViews[accessor.bufferView];
			const tinygltf::Buffer&		buffer		= model.buffers[bufferView.buffer];

			// reading indices
			if (primitive.indices >= 0)
			{
				const unsigned char* pData = buffer.data.data() + bufferView.byteOffset + accessor.byteOffset;
				size_t indexCount = static_cast<int>(accessor.count);

				std::cout << "number of indices: " << indexCount << "\n";

				if (accessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT)
				{
					const uint16_t* indices = reinterpret_cast<const uint16_t*>(pData);
					_rOutMeshData.indices16.reserve(indexCount);
					_rOutMeshData.indices16.insert(_rOutMeshData.indices16.end(), indices, indices + indexCount);
				}
				else
				{
					std::cout << "Indextype not supported. Has to be uint16!";
				}
			}

			//reading vertecis
			std::map<std::string, int>::const_iterator posAttribute = primitive.attributes.find("POSITION");
			if (posAttribute != primitive.attributes.end())
			{
				int accessorIndex = posAttribute->second; 

				const tinygltf::Accessor&	accessor	= model.accessors[accessorIndex]; 
				const tinygltf::BufferView	bufferView	= model.bufferViews[accessor.bufferView];
				const tinygltf::Buffer&		buffer		= model.buffers[bufferView.buffer];

				const unsigned char* pData = buffer.data.data() + bufferView.byteOffset + accessor.byteOffset;
			
				size_t vertexCount = accessor.count; 

				std::cout << "positions count " << vertexCount << "\n";

				if (accessor.componentType == TINYGLTF_COMPONENT_TYPE_FLOAT)
				{
					const float* positions = reinterpret_cast<const float*>(pData);
					
					_rOutMeshData.vertices.resize(vertexCount);

					for (int index = 0; index < vertexCount; index++)
					{
						_rOutMeshData.vertices[index].position.x = positions[index * 3 + 0];
						_rOutMeshData.vertices[index].position.y = positions[index * 3 + 1];
						_rOutMeshData.vertices[index].position.z = positions[index * 3 + 2];

						std::cout << positions[index * 3 + 0] << ", " << positions[index * 3 + 1] << ", " << positions[index * 3 + 2] << "\n";
					}
				}
			}


			// reading normals 
			std::map<std::string, int>::const_iterator normAttribute = primitive.attributes.find("NORMAL");
			if (normAttribute != primitive.attributes.end())
			{
				int accessorIndex = normAttribute->second;

				const tinygltf::Accessor&	accessor	= model.accessors[accessorIndex];
				const tinygltf::BufferView	bufferView	= model.bufferViews[accessor.bufferView];
				const tinygltf::Buffer&		buffer		= model.buffers[bufferView.buffer];

				const unsigned char* pData = buffer.data.data() + bufferView.byteOffset + accessor.byteOffset;

				size_t normalCount = accessor.count;

				std::cout << "positions count " << normalCount << "\n";

				if (accessor.componentType == TINYGLTF_COMPONENT_TYPE_FLOAT)
				{
					const float* normals = reinterpret_cast<const float*>(pData);
				

					_rOutMeshData.vertices.resize(normalCount);
					
					std::cout << "normals" << std::endl;
					for (int index = 0; index < normalCount; index++)
					{
						_rOutMeshData.vertices[index].normal.x = normals[index * 3 + 0];
						_rOutMeshData.vertices[index].normal.y = normals[index * 3 + 1];
						_rOutMeshData.vertices[index].normal.z = normals[index * 3 + 2];

						std::cout << normals[index * 3 + 0] << ", " << normals[index * 3 + 1] << ", " << normals[index * 3 + 2] << "\n";
					}
				}
			}

		}
	}
}

// --------------------------------------------------------------------------------------------------------------------------

void cMeshGenerator::LoadModelFromGLTF(std::string& _rFilePath, std::vector<sMeshData>& _rOutMeshData, std::vector<XMMATRIX>& _rOutWorldMatrix)
{
	tinygltf::Model model;
	tinygltf::TinyGLTF loader;
	std::string err, warn;

	bool ret;
	if (_rFilePath.substr(_rFilePath.size() - 4) == ".glb")
	{
		ret = loader.LoadBinaryFromFile(&model, &err, &warn, _rFilePath);
	}
	else
	{
		ret = loader.LoadASCIIFromFile(&model, &err, &warn, _rFilePath);
	}

	if (!warn.empty())
		std::cout << "GLTF Warning: " << warn << std::endl;
	if (!err.empty())
		std::cerr << "GLTF Error: " << err << std::endl;
	if (!ret)
	{
		std::cerr << "Failed to load GLTF: " << _rFilePath << std::endl;
		return;
	}

	std::cout << "Number of Nodes: " << model.nodes.size() << "\n";

	// Process each root node in the default scene
	if (model.scenes.empty())
	{
		std::cerr << "GLTF Error: No scenes found.\n";
		return;
	}

	const tinygltf::Scene& scene = model.scenes[model.defaultScene >= 0 ? model.defaultScene : 0];

	for (int rootNodeIndex : scene.nodes)
	{
		// Start with identity matrix for each root node
		XMMATRIX identity = XMMatrixIdentity();

		// Recursively process the node hierarchy
		ProcessNode(model, rootNodeIndex, identity, _rOutMeshData, _rOutWorldMatrix);
	}
}

// --------------------------------------------------------------------------------------------------------------------------

void cMeshGenerator::ProcessNode(tinygltf::Model& _rModel, int _nodeIndex, XMMATRIX _parentWorldMatrix, std::vector<sMeshData>& _rOutMeshData, std::vector<XMMATRIX>& _rOutWorldMatrix)
{
	const tinygltf::Node& node = _rModel.nodes[_nodeIndex];

	// Compute local transform matrix from matrix or TRS
	XMMATRIX localMatrix = XMMatrixIdentity();

	if (node.matrix.size() == 16)
	{
		localMatrix = XMMatrixSet(
			static_cast<float>(node.matrix[0]), static_cast<float>(node.matrix[1]), static_cast<float>(node.matrix[2]), static_cast<float>(node.matrix[3]),
			static_cast<float>(node.matrix[4]), static_cast<float>(node.matrix[5]), static_cast<float>(node.matrix[6]), static_cast<float>(node.matrix[7]),
			static_cast<float>(node.matrix[8]), static_cast<float>(node.matrix[9]), static_cast<float>(node.matrix[10]), static_cast<float>(node.matrix[11]),
			static_cast<float>(node.matrix[12]), static_cast<float>(node.matrix[13]), static_cast<float>(node.matrix[14]), static_cast<float>(node.matrix[15])
		);
	}
	else
	{
		XMVECTOR translation = XMVectorSet(
			node.translation.size() > 0 ? static_cast<float>(node.translation[0]) : 0.f,
			node.translation.size() > 1 ? static_cast<float>(node.translation[1]) : 0.f,
			node.translation.size() > 2 ? static_cast<float>(node.translation[2]) : 0.f,
			0.f);

		XMVECTOR rotation = XMQuaternionIdentity();
		if (node.rotation.size() == 4)
		{
			rotation = XMVectorSet(
				static_cast<float>(node.rotation[0]),
				static_cast<float>(node.rotation[1]),
				static_cast<float>(node.rotation[2]),
				static_cast<float>(node.rotation[3]));
		}

		XMVECTOR scale = XMVectorSet(
			node.scale.size() > 0 ? static_cast<float>(node.scale[0]) : 1.f,
			node.scale.size() > 1 ? static_cast<float>(node.scale[1]) : 1.f,
			node.scale.size() > 2 ? static_cast<float>(node.scale[2]) : 1.f,
			0.f);

		localMatrix = XMMatrixAffineTransformation(scale, XMVectorZero(), rotation, translation);
	}

	// Combine with parent transform to get world matrix
	XMMATRIX worldMatrix = localMatrix * _parentWorldMatrix;

	// If node has mesh, extract and store mesh data with its world matrix
	if (node.mesh >= 0)
	{
		sMeshData meshData;
		ExtractPrimitives(_rModel, node.mesh, meshData);

		_rOutMeshData.push_back(std::move(meshData));
		_rOutWorldMatrix.push_back(worldMatrix);
	}

	// Recursively process children with current world matrix
	for (int childIndex : node.children)
	{
		ProcessNode(_rModel, childIndex, worldMatrix, _rOutMeshData, _rOutWorldMatrix);
	}
}

// --------------------------------------------------------------------------------------------------------------------------

void cMeshGenerator::ExtractPrimitives(tinygltf::Model& model, int meshIndex, sMeshData& outMeshData)
{
	const tinygltf::Mesh& mesh = model.meshes[meshIndex];

	for (const auto& primitive : mesh.primitives)
	{
		// --- Extract indices (only uint16 supported here) ---
		if (primitive.indices >= 0)
		{
			const tinygltf::Accessor& accessor = model.accessors[primitive.indices];
			const tinygltf::BufferView& bufferView = model.bufferViews[accessor.bufferView];
			const tinygltf::Buffer& buffer = model.buffers[bufferView.buffer];

			const unsigned char* pData = buffer.data.data() + bufferView.byteOffset + accessor.byteOffset;
			size_t indexCount = static_cast<size_t>(accessor.count);

			std::cout << "number of indices: " << indexCount << "\n";

			if (accessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT)
			{
				const uint16_t* indices = reinterpret_cast<const uint16_t*>(pData);
				// Append indices, adjusting for current vertex count
				size_t vertexOffset = outMeshData.vertices.size();
				outMeshData.indices16.reserve(outMeshData.indices16.size() + indexCount);
				for (size_t i = 0; i < indexCount; ++i)
				{
					outMeshData.indices16.push_back(indices[i] + static_cast<uint16_t>(vertexOffset));
				}
			}
			if (accessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT)
			{
				const uint32_t* indices = reinterpret_cast<const uint32_t*>(pData);
				// Append indices, adjusting for current vertex count
				size_t vertexOffset = outMeshData.vertices.size();
				outMeshData.indices32.reserve(outMeshData.indices32.size() + indexCount);
				for (size_t i = 0; i < indexCount; ++i)
				{
					outMeshData.indices32.push_back(indices[i] + static_cast<uint32_t>(vertexOffset));
				}
			}
			else
			{
				std::cout << "Index type not supported. Has to be uint16!\n";
			}
		}

		// --- Extract positions ---
		auto posIt = primitive.attributes.find("POSITION");
		if (posIt != primitive.attributes.end())
		{
			int accessorIndex = posIt->second;

			const tinygltf::Accessor& accessor = model.accessors[accessorIndex];
			const tinygltf::BufferView& bufferView = model.bufferViews[accessor.bufferView];
			const tinygltf::Buffer& buffer = model.buffers[bufferView.buffer];

			const unsigned char* pData = buffer.data.data() + bufferView.byteOffset + accessor.byteOffset;

			size_t vertexCount = static_cast<size_t>(accessor.count);

			std::cout << "positions count " << vertexCount << "\n";

			if (accessor.componentType == TINYGLTF_COMPONENT_TYPE_FLOAT)
			{
				const float* positions = reinterpret_cast<const float*>(pData);
				// Append vertices (resize once for new vertices)
				size_t oldVertexCount = outMeshData.vertices.size();
				outMeshData.vertices.resize(oldVertexCount + vertexCount);

				for (size_t i = 0; i < vertexCount; ++i)
				{
					outMeshData.vertices[oldVertexCount + i].position.x = positions[i * 3 + 0];
					outMeshData.vertices[oldVertexCount + i].position.y = positions[i * 3 + 1];
					outMeshData.vertices[oldVertexCount + i].position.z = positions[i * 3 + 2];
				}
			}
		}

		// --- Extract normals ---
		auto normIt = primitive.attributes.find("NORMAL");
		if (normIt != primitive.attributes.end())
		{
			int accessorIndex = normIt->second;

			const tinygltf::Accessor& accessor = model.accessors[accessorIndex];
			const tinygltf::BufferView& bufferView = model.bufferViews[accessor.bufferView];
			const tinygltf::Buffer& buffer = model.buffers[bufferView.buffer];

			const unsigned char* pData = buffer.data.data() + bufferView.byteOffset + accessor.byteOffset;

			size_t normalCount = static_cast<size_t>(accessor.count);

			std::cout << "normals count " << normalCount << "\n";

			if (accessor.componentType == TINYGLTF_COMPONENT_TYPE_FLOAT)
			{
				const float* normals = reinterpret_cast<const float*>(pData);

				// Assume normals count matches vertex count and append accordingly
				size_t startIndex = outMeshData.vertices.size() - normalCount;
				for (size_t i = 0; i < normalCount; ++i)
				{
					outMeshData.vertices[startIndex + i].normal.x = normals[i * 3 + 0];
					outMeshData.vertices[startIndex + i].normal.y = normals[i * 3 + 1];
					outMeshData.vertices[startIndex + i].normal.z = normals[i * 3 + 2];
				}
			}
		}
	}
}
