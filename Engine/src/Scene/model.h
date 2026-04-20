#pragma once

#include <vector>

#include "graphics/meshData.h"
#include "graphics/material.h"
#include "graphics/cpuTexture.h"

struct sModel
{
    std::vector<sMeshData>      meshes;
    std::vector<sMaterial>      materials;
    std::vector<XMMATRIX>       worldMatrices;
    std::vector<cCpuTexture>    cpuTextures;
};