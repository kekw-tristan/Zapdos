// Vertex Shader (VS)
cbuffer cbPerObject : register(b0)
{
    float4x4 gWorldViewProj;
}

struct sVertexIn
{
    float3 pos : POSITION;
    float4 color : COLOR;
};

struct sVertexOut
{
    float4 pos : SV_POSITION; // Change to float4 for correct SV_POSITION
    float4 color : COLOR;
};

sVertexOut VS(sVertexIn _vertex)
{
    sVertexOut vertex;
    vertex.pos = mul(float4(_vertex.pos, 1.0f), gWorldViewProj);
    vertex.color = _vertex.color;
    return vertex;
}

// Pixel Shader (PS)
struct sPixelIn
{
    float4 pos : SV_POSITION;
    float4 color : COLOR;
};

float4 PS(sPixelIn _pixel) : SV_Target
{
    return _pixel.color; // Simply return the color passed from vertex shader
}
