static const float PI = 3.14159265359f;

// === Constant Buffers ===
cbuffer cbPerObject : register(b0)
{
    // Object transform
    float4x4 gWorld;             
    float4x4 gWorldInvTranspose;

    // PBR material properties
    float4  gBaseColor;     // RGB = Albedo,    A = Alpha/Opacity
    float   gMetallic;      // 0 = dielectric,  1 = metal
    float   gRoughness;     // 0 = smooth,      1 = rough
    float   gAO;            // Ambient occlusion factor
    float   padding1;
    float3  gEmissive;      // Emissive color
    float   padding2;      
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
    int type; // 0 = directional, 1 = point, 2 = spot
    float3 padding;
};

StructuredBuffer<sLight> gLights : register(t0);

// === Vertex Input ===
struct sVertexIn
{
    float3 pos : POSITION;
    float3 normal : NORMAL;
    float4 tangentU : TANGENT; // optional, can ignore in constant-only PBR
    float2 texC : TEXCOORD0; // optional
    float2 texC2 : TEXCOORD1; // optional
};

// Vertex Shader Output
struct sVertexOut
{
    float4 pos : SV_POSITION; // clip-space position
    float3 normalW : NORMAL; // world-space normal
    float3 posW : POSITION1; // world-space position
    float2 texC : TEXCOORD0; // passed along for future textures
    float2 texC2 : TEXCOORD1; // optional second UV set
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
    vout.normalW = normalize(mul((float3x3) gWorldInvTranspose, vin.normal));

    // Pass through texcoords for future textures
    vout.texC = vin.texC;
    vout.texC2 = vin.texC2;

    return vout;
}

// === Pixel Shader ===
float3 FresnelSchlick(float cosTheta, float3 F0)
{
    return F0 + (1.0f - F0) * pow(1.0f - cosTheta, 5.0f);
}

// Normal Distribution Function (GGX/Trowbridge-Reitz)
float DistributionGGX(float3 N, float3 H, float roughness)
{
    float a = roughness * roughness;
    float a2 = a * a;
    float NdotH = saturate(dot(N, H));
    float NdotH2 = NdotH * NdotH;

    float denom = NdotH2 * (a2 - 1.0f) + 1.0f;
    return a2 / (PI * denom * denom + 1e-5);
}

// Geometry function (Schlick-GGX)
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

float3 PS(sVertexOut pin) : SV_Target
{
    float3 N = normalize(pin.normalW);
    float3 V = normalize(gEyePosW - pin.posW);

    // Calculate F0 from base color and metallic
    float3 F0 = lerp(float3(0.04f, 0.04f, 0.04f), gBaseColor.rgb, gMetallic);

    float3 Lo = float3(0, 0, 0); // outgoing radiance

    // Loop through lights
    for (int i = 0; i < gLightCount; ++i)
    {
        sLight light = gLights[i];

        float3 L = float3(0, 0, 0);
        float attenuation = 1.0f;

        if (light.type == 0) // Directional
        {
            L = normalize(-light.direction);
            attenuation = 1.0f;
        }
        else
        {
            float3 lightVec = light.position - pin.posW;
            float dist = length(lightVec);
            L = normalize(lightVec);

            float falloffRange = max(light.falloffEnd - light.falloffStart, 0.001f);
            attenuation = saturate(1.0f - saturate((dist - light.falloffStart) / falloffRange));
            attenuation *= attenuation;

            if (light.type == 2) // Spot
            {
                float3 spotDir = normalize(light.direction);
                float spotCos = dot(-L, spotDir);
                float spotEffect = saturate(pow(spotCos, light.spotPower));
                attenuation *= spotEffect;
            }
        }

        float3 H = normalize(V + L);
        float NdotL = saturate(dot(N, L));
        float NdotV = saturate(dot(N, V));
        float NdotH = saturate(dot(N, H));
        float VdotH = saturate(dot(V, H));

        // Cook-Torrance BRDF
        float D = DistributionGGX(N, H, gRoughness);
        float G = GeometrySmith(N, V, L, gRoughness);
        float3 F = FresnelSchlick(VdotH, F0);

        float3 numerator = D * G * F;
        float denominator = 4.0f * NdotV * NdotL + 1e-5;
        float3 specular = numerator / denominator;

        // kS = specular, kD = diffuse component
        float3 kS = F;
        float3 kD = float3(1.0f, 1.0f, 1.0f) - kS;
        kD *= 1.0f - gMetallic;

        float3 diffuse = kD * gBaseColor.rgb / PI;

        Lo += (diffuse + specular) * light.strength * NdotL * attenuation;
    }

    // Ambient / AO
    float3 ambient = 0.03f * gBaseColor.rgb * gAO;

    float3 color = ambient + Lo + gEmissive;

    // HDR tonemapping (optional)
    color = color / (color + 1.0f);

    // Gamma correction
    color = pow(color, float3(1.0f / 2.2f, 1.0f / 2.2f, 1.0f / 2.2f));

    return float4(color, gBaseColor.a);
}