// Vertex Shader (VS)
cbuffer cbPerObject : register(b0)
{
    float4x4 gWorld;
}

cbuffer cbPass : register(b1)
{
    float4x4    gView;
    float4x4    gInvView;
    float4x4    gProj;
    float4x4    gInvProj;
    float4x4    gViewProj;
    float4x4    gInvViewProj;
    float3      gEyePosW;
    float       cbPerObjectPad1;
    float2      gRenderTargetSize;
    float2      gInvRenderTargetSize;
    float       gNearZ;
    float       gFarZ;
    float       gTotalTime;
    float       gDeltaTime;
}
struct sVertexIn
{
    float3 pos : POSITION;
    float4 color : COLOR;
};

struct sVertexOut
{
    float4 pos : SV_POSITION;     
    float4 color : COLOR;
};


sVertexOut VS(sVertexIn _vertex)
{
    sVertexOut vertex;
    
    // transform to homogeneus space
    float4 posW = mul(float4(_vertex.pos, 1.0f), gWorld);
    vertex.pos = mul(posW, gViewProj);

    vertex.color = _vertex.color;
    return vertex;
}

// Pixel Shader (PS)
struct sPixelIn
{
    float4 pos : SV_POSITION;
    float4 color : COLOR;
};

float4 PS(sVertexOut _pixel) : SV_Target
{
   return _pixel.color; 
}
