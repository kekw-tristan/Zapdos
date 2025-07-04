// === Constant Buffers ===
cbuffer cbPerObject : register(b0)
{
    float4x4 gWorld;
    float3   gAlbedo;
    float    gSpecularExponent;
};

cbuffer cbPass : register(b1)
{
    float4x4 gView;
    float4x4 gInvView;
    float4x4 gProj;
    float4x4 gInvProj;
    float4x4 gViewProj;
    float4x4 gInvViewProj;
    float3   gEyePosW;
    float    cbPassPad1;  // Padding
    float2   gRenderTargetSize;
    float2   gInvRenderTargetSize;
    float    gNearZ;
    float    gFarZ;
    float    gTotalTime;
    float    gDeltaTime;
};

cbuffer cbLight : register(b2)
{
    float3  gStrength;
    float   gFalloffStart;
    float3  gDirection;
    float   gFalloffEnd;
    float3  gPosition;
    float   gSpotPower;
}

// === Vertex Input ===
struct sVertexIn
{
    float3 pos    : POSITION;
    float3 normal : NORMAL;
};

// === Vertex Output ===
struct sVertexOut
{
    float4 pos     : SV_POSITION;
    float3 normalW : NORMAL;
    float3 posW    : POSITION1;
};

// === Vertex Shader ===
sVertexOut VS(sVertexIn vin)
{
    sVertexOut vout;

    // World position
    float4 posW = mul(float4(vin.pos, 1.0f), gWorld);
    vout.posW = posW.xyz;

    // Clip space
    vout.pos = mul(posW, gViewProj);

    // World normal
    vout.normalW = normalize(mul((float3x3)gWorld, vin.normal));

    return vout;
}

// === Pixel Shader ===
float4 PS(sVertexOut pin) : SV_Target
{
    float3 N = normalize(pin.normalW);              // Surface normal at the pixel
    float3 L = normalize(-gDirection);              // Directional light vector (note: negative of gDirection)
    float3 V = normalize(gEyePosW - pin.posW);      // Vector from pixel to eye
    float3 H = normalize(L + V);                    // Half vector = normalize(L + V)

    // Ambient
    float3 ambient = gAlbedo * 0.3f;

    // Diffuse (Lambert)
    float NdotL = saturate(dot(N, L));
    float3 diffuse = gAlbedo * gStrength * NdotL;

    // Specular (Blinn-Phong)
    float NdotH = saturate(dot(N, H));
    float spec = pow(NdotH, max(gSpecularExponent, 1.0f));
    float3 specular = gStrength * spec * 0.3f;

    float3 finalColor = ambient + diffuse + specular;
    return float4(finalColor, 1.0f);
}
