// === Constant Buffers ===
cbuffer cbPerObject : register(b0)
{
    float4x4 gWorld;
};

cbuffer cbPass : register(b1)
{
    float4x4 gView;
    float4x4 gInvView;
    float4x4 gProj;
    float4x4 gInvProj;
    float4x4 gViewProj;
    float4x4 gInvViewProj;
    float3 gEyePosW;
    float cbPassPad1; // Pad to keep alignment
    float2 gRenderTargetSize;
    float2 gInvRenderTargetSize;
    float gNearZ;
    float gFarZ;
    float gTotalTime;
    float gDeltaTime;
};

// === Vertex Input ===
struct sVertexIn
{
    float3 pos : POSITION;
    float4 color : COLOR;
};

// === Vertex Output ===
struct sVertexOut
{
    float4 pos : SV_POSITION;
    float4 color : COLOR;
};

// === Vertex Shader ===
sVertexOut VS(sVertexIn vin)
{
    sVertexOut vout;

    float4 posW = mul(float4(vin.pos, 1.0f), gWorld);
    vout.pos = mul(posW, gViewProj);
    vout.color = vin.color;

    return vout;
}

// === Pixel Shader ===
float4 PS(sVertexOut pin) : SV_Target
{
    return pin.color;
}