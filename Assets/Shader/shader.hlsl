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
    int gBaseColorIndex; // -1 = keine Texture
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
    float spotPower;
    int type; // 0=directional,1=point,2=spot
    float3 padding;
};

StructuredBuffer<sLight> gLights : register(t0);

// t1 .. t128
Texture2D textures[512] : register(t1);
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
    vout.normalW = normalize(mul((float3x3) gWorldInvTranspose, vin.normal));
    vout.texC = vin.texC;
    vout.texC2 = vin.texC2;

    return vout;
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
    return a2 / (PI * denom * denom + 1e-5);
}

float GeometrySchlickGGX(float NdotV, float roughness)
{
    float r = roughness + 1.0f;
    float k = (r * r) / 8.0f;
    return NdotV / (NdotV * (1.0f - k) + k + 1e-5);
}

float GeometrySmith(float3 N, float3 V, float3 L, float roughness)
{
    float NdotV = saturate(dot(N, V));
    float NdotL = saturate(dot(N, L));
    return GeometrySchlickGGX(NdotV, roughness) * GeometrySchlickGGX(NdotL, roughness);
}

float4 SampleBaseColorTexture(int index, float2 uv)
{
    float4 result = float4(1.0f, 1.0f, 1.0f, 1.0f);

    if (index >= 0 && index < 128)
    {
        result = textures[NonUniformResourceIndex((uint) index)].Sample(samp, uv);
    }

    return result;
}

// === Pixel Shader ===
float4 PS(sVertexOut pin) : SV_Target
{
    float3 N = normalize(pin.normalW);
    float3 V = normalize(gEyePosW - pin.posW);

    float4 baseTex = SampleBaseColorTexture(gBaseColorIndex, pin.texC);

    float3 albedo = gBaseColor.rgb * baseTex.rgb;
    float alpha = gBaseColor.a * baseTex.a;

    float roughness = saturate(gRoughness);
    float metallic = saturate(gMetallic);
    float ao = saturate(gAO);

    float3 F0 = lerp(float3(0.04f, 0.04f, 0.04f), albedo, metallic);

    float3 Lo = float3(0.0f, 0.0f, 0.0f);

    [loop]
    for (int i = 0; i < gLightCount; ++i)
    {
        sLight light = gLights[i];

        float3 L = float3(0.0f, 0.0f, 0.0f);
        float attenuation = 1.0f;

        if (light.type == 0)
        {
            L = normalize(-light.direction);
        }
        else
        {
            float3 lightVec = light.position - pin.posW;
            float dist = length(lightVec);
            L = normalize(lightVec);

            float falloffRange = max(light.falloffEnd - light.falloffStart, 0.001f);
            attenuation = saturate(1.0f - saturate((dist - light.falloffStart) / falloffRange));
            attenuation *= attenuation;

            if (light.type == 2)
            {
                float3 spotDir = normalize(light.direction);
                float spotCos = dot(-L, spotDir);
                attenuation *= saturate(pow(spotCos, light.spotPower));
            }
        }

        float3 H = normalize(V + L);
        float NdotL = saturate(dot(N, L));
        float NdotV = saturate(dot(N, V));
        float VdotH = saturate(dot(V, H));

        float D = DistributionGGX(N, H, roughness);
        float G = GeometrySmith(N, V, L, roughness);
        float3 F = FresnelSchlick(VdotH, F0);

        float3 kS = F;
        float3 kD = 1.0f.xxx - kS;
        kD *= (1.0f - metallic);

        float3 diffuse = kD * albedo / PI;
        float3 specular = (D * G * F) / max(NdotV * NdotL * 4.0f, 1e-5f);

        Lo += (diffuse + specular) * light.strength * attenuation * NdotL;
    }

    float3 ambient = 0.03f * albedo * ao;
    float3 color = ambient + Lo + gEmissive;

    // Tonemap
    color = color / (color + 1.0f.xxx);

    // Gamma
    color = pow(color, (1.0f / 2.2f).xxx);

    return float4(color, alpha);
}