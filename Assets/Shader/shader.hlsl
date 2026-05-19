#include "../../Engine/src/Graphics/gfxConfig.h"

static const float PI = 3.14159265359f;

// === Constant Buffers ===
cbuffer cbPerObject : register(b0)
{
    float4x4 gWorld;
    float4x4 gWorldInvTranspose;

    float4 gBaseColor; // RGB = Albedo factor, A = Alpha factor

    float gMetallic;
    float gRoughness;
    float gAO;
    float padding1;

    float3 gEmissive;
    float gEmissiveStrength;
    
    int gBaseColorIndex; // -1 = keine Texture
    int gMetallicRoughnessIndex;
    int gNormalIndex;
    int gOcclusionIndex;
    
    int gEmissiveIndex;
    float gNormalScale;
    float gOcclusionStrength;
    int padding2;
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
    int gLightCount;

    float2 gRenderTargetSize;
    float2 gInvRenderTargetSize;

    float gNearZ;
    float gFarZ;
    float gTotalTime;
    float gDeltaTime;
};

struct sLight
{
    float3 strength;
    float falloffStart;

    float3 direction;
    float falloffEnd;

    float3 position;
    float spotInnerConeCos;

    int type;
    float spotOuterConeCos;
    float2 padding;
};

StructuredBuffer<sLight> gLights : register(t0);

// t1 .. tN
Texture2D textures[GFX_MAX_NUMGER_OF_TEXTURES] : register(t1);
SamplerState samp : register(s0);

// === Vertex Input / Output ===
struct sVertexIn
{
    float3 pos : POSITION;
    float3 normal : NORMAL;
    float4 tangentU : TANGENT;
    float2 texC : TEXCOORD0;
    float2 texC2 : TEXCOORD1;
};

struct sVertexOut
{
    float4 pos : SV_POSITION;
    float3 normalW : NORMAL;
    float4 tangentW : TANGENT;
    float3 posW : POSITION1;
    float2 texC : TEXCOORD0;
    float2 texC2 : TEXCOORD1;
};

// === Vertex Shader ===
sVertexOut VS(sVertexIn vin)
{
    sVertexOut vout;

    float4 posW = mul(float4(vin.pos, 1.0f), gWorld);

    vout.posW = posW.xyz;
    vout.pos = mul(posW, gViewProj);

    // Bei deiner mul-Reihenfolge: vector * matrix
    float3 normalW = mul(vin.normal, (float3x3) gWorldInvTranspose);
    float3 tangentW = mul(vin.tangentU.xyz, (float3x3) gWorld);

    vout.normalW = normalize(normalW);
    vout.tangentW = float4(normalize(tangentW), vin.tangentU.w);

    vout.texC = vin.texC;
    vout.texC2 = vin.texC2;

    return vout;
}

// === Texture Sampling Helper ===
float4 SampleTextureByIndex(int index, float2 uv, float4 defaultValue)
{
    if (index >= 0 && index < GFX_MAX_NUMGER_OF_TEXTURES)
    {
        return textures[NonUniformResourceIndex((uint) index)].Sample(samp, uv);
    }

    return defaultValue;
}

// === Normal Mapping ===
float3 GetNormalW(float3 normalW, float4 tangentW, float2 uv)
{
    float3 N = normalize(normalW);

    if (gNormalIndex < 0)
        return N;

    float3 T = tangentW.xyz;

    if (dot(T, T) < 1e-6f)
        return N;

    T = normalize(T);

    // Gram-Schmidt: Tangent orthogonal zur Normal machen
    T = normalize(T - N * dot(T, N));

    float3 B = normalize(cross(N, T) * tangentW.w);

    float3 normalTex = SampleTextureByIndex(
        gNormalIndex,
        uv,
        float4(0.5f, 0.5f, 1.0f, 1.0f)
    ).xyz;

    // [0,1] -> [-1,1]
    float3 tangentNormal = normalTex * 2.0f - 1.0f;

    // glTF normalTexture.scale skaliert X/Y
    tangentNormal.xy *= gNormalScale;
    tangentNormal = normalize(tangentNormal);

    float3x3 TBN = float3x3(T, B, N);

    return normalize(mul(tangentNormal, TBN));
}

// === PBR Helper ===
float3 FresnelSchlick(float cosTheta, float3 F0)
{
    return F0 + (1.0f - F0) * pow(1.0f - cosTheta, 5.0f);
}

float DistributionGGX(float3 N, float3 H, float roughness)
{
    float a = roughness * roughness;
    float a2 = a * a;

    float NdotH = saturate(dot(N, H));
    float NdotH2 = NdotH * NdotH;

    float denom = NdotH2 * (a2 - 1.0f) + 1.0f;

    return a2 / max(PI * denom * denom, 1e-5f);
}

float GeometrySchlickGGX(float NdotV, float roughness)
{
    float r = roughness + 1.0f;
    float k = (r * r) / 8.0f;

    return NdotV / max(NdotV * (1.0f - k) + k, 1e-5f);
}

float GeometrySmith(float3 N, float3 V, float3 L, float roughness)
{
    float NdotV = saturate(dot(N, V));
    float NdotL = saturate(dot(N, L));

    float ggxV = GeometrySchlickGGX(NdotV, roughness);
    float ggxL = GeometrySchlickGGX(NdotL, roughness);

    return ggxV * ggxL;
}

// === Pixel Shader ===
float4 PS(sVertexOut pin) : SV_Target
{
    float3 N = GetNormalW(pin.normalW, pin.tangentW, pin.texC);
    float3 V = normalize(gEyePosW - pin.posW);

    // ------------------------------------------------------------
    // Base Color
    // glTF: baseColor = baseColorFactor * baseColorTexture
    // ------------------------------------------------------------
    float4 baseTex = SampleTextureByIndex(
        gBaseColorIndex,
        pin.texC,
        float4(1.0f, 1.0f, 1.0f, 1.0f)
    );

    float3 albedo = gBaseColor.rgb * baseTex.rgb;
    float alpha = gBaseColor.a * baseTex.a;

    // ------------------------------------------------------------
    // Metallic-Roughness
    //
    // glTF Packing:
    // R = unused
    // G = roughness
    // B = metallic
    // A = unused
    // ------------------------------------------------------------
    float4 metallicRoughnessTex = SampleTextureByIndex(
        gMetallicRoughnessIndex,
        pin.texC,
        float4(1.0f, 1.0f, 1.0f, 1.0f)
    );

    float roughness = saturate(gRoughness * metallicRoughnessTex.g);
    float metallic = saturate(gMetallic * metallicRoughnessTex.b);

    roughness = max(roughness, 0.04f);

    // ------------------------------------------------------------
    // Occlusion
    //
    // glTF Packing:
    // R = ambient occlusion
    // ------------------------------------------------------------
    float occlusionSample = SampleTextureByIndex(
        gOcclusionIndex,
        pin.texC,
        float4(1.0f, 1.0f, 1.0f, 1.0f)
    ).r;

    float ao = lerp(1.0f, occlusionSample, saturate(gOcclusionStrength));
    ao = saturate(ao * gAO);

    // ------------------------------------------------------------
    // Emissive
    // ------------------------------------------------------------
    float3 emissiveTex = SampleTextureByIndex(
        gEmissiveIndex,
        pin.texC,
        float4(1.0f, 1.0f, 1.0f, 1.0f)
    ).rgb;

    float3 emissive = gEmissive * emissiveTex * gEmissiveStrength;

    // ------------------------------------------------------------
    // PBR Lighting
    // ------------------------------------------------------------
    float3 F0 = lerp(
        float3(0.04f, 0.04f, 0.04f),
        albedo,
        metallic
    );

    float3 Lo = float3(0.0f, 0.0f, 0.0f);

    for (int i = 0; i < gLightCount; ++i)
    {
        sLight light = gLights[i];

        float3 L = float3(0.0f, 0.0f, 0.0f);
        float attenuation = 1.0f;

        if (light.type == 0)
        {
            // Directional light
            L = normalize(-light.direction);
        }
        else
        {
            // Point / Spot light
            float3 lightVec = light.position - pin.posW;
            float dist = length(lightVec);

            L = lightVec / max(dist, 1e-5f);

            float falloffRange = max(light.falloffEnd - light.falloffStart, 0.001f);
            attenuation = saturate(1.0f - ((dist - light.falloffStart) / falloffRange));
            attenuation *= attenuation;

            if (light.type == 2)
            {
                float3 spotDir = normalize(light.direction);

                float spotCos = dot(-L, spotDir);

                float spotAttenuation = saturate(
                    (spotCos - light.spotOuterConeCos) /
                    max(light.spotInnerConeCos - light.spotOuterConeCos, 1e-5f)
                );

                attenuation *= spotAttenuation * spotAttenuation;
            }
        }

        float3 H = normalize(V + L);

        float NdotL = saturate(dot(N, L));
        float NdotV = saturate(dot(N, V));
        float VdotH = saturate(dot(V, H));

        if (NdotL <= 0.0f)
            continue;

        float D = DistributionGGX(N, H, roughness);
        float G = GeometrySmith(N, V, L, roughness);
        float3 F = FresnelSchlick(VdotH, F0);

        float3 kS = F;
        float3 kD = float3(1.0f, 1.0f, 1.0f) - kS;
        kD *= 1.0f - metallic;

        float3 diffuse = kD * albedo / PI;

        float3 numerator = D * G * F;
        float denominator = max(4.0f * NdotV * NdotL, 1e-5f);
        float3 specular = numerator / denominator;

        Lo += (diffuse + specular) * light.strength * attenuation * NdotL;
    }

    float3 ambientColor = float3(0.006f, 0.008f, 0.014f);
    float3 ambient = ambientColor * albedo * ao;

    float3 color = ambient + Lo + emissive;

    // Tonemap
    color = color / (color + float3(1.0f, 1.0f, 1.0f));

    // Gamma
    color = pow(color, float3(1.0f / 2.2f, 1.0f / 2.2f, 1.0f / 2.2f));

    return float4(color, alpha);
}