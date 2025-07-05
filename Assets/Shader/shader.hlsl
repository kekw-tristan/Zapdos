// === Constant Buffers ===
cbuffer cbPerObject : register(b0)
{
    float4x4 gWorld;
    float3 gAlbedo;
    float gSpecularExponent;
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
};

// === Vertex Output ===
struct sVertexOut
{
    float4 pos : SV_POSITION;
    float3 normalW : NORMAL;
    float3 posW : POSITION1;
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
    vout.normalW = normalize(mul((float3x3) gWorld, vin.normal));

    return vout;
}

// === Pixel Shader ===
float4 PS(sVertexOut pin) : SV_Target
{
    float3 N = normalize(pin.normalW);
    float3 V = normalize(gEyePosW - pin.posW);

    float3 ambient = gAlbedo * 0.3f;
    float3 diffuse = float3(0, 0, 0);
    float3 specular = float3(0, 0, 0);

    for (int i = 0; i < gLightCount; ++i)
    {
        sLight light = gLights[i];
        
        float3 L = float3(0, 0, 0);
        float attenuation = 1.0f;
        float spotEffect = 1.0f;

        if (light.type == 0) // Directional Light
        {
            // Directional lights have a direction TO the surface
            L = normalize(-light.direction);
            attenuation = 1.0f; // No falloff for directional lights
        }
        else
        {
            // For point and spot lights: vector from pixel to light position
            float3 lightVec = light.position - pin.posW;
            float dist = length(lightVec);
            L = normalize(lightVec);

            // Attenuation based on distance falloff
            float falloffRange = max(light.falloffEnd - light.falloffStart, 0.001f);
            attenuation = saturate(1.0f - saturate((dist - light.falloffStart) / falloffRange));
            attenuation *= attenuation; // quadratic falloff

            if (light.type == 2) // Spot Light
            {
                // For spotlights, direction is from light position OUTWARD along light.direction
                float3 spotDir = normalize(light.direction);
                
                // Calculate angle between light direction and vector to pixel (pointing back at light)
                float spotCos = dot(-L, spotDir); // L points from pixel to light, invert for cone check
                
                // Apply spotlight cone attenuation
                spotEffect = saturate(pow(spotCos, light.spotPower));
                attenuation *= spotEffect;
            }
        }

        // Lambert Diffuse
        float NdotL = saturate(dot(N, L));
        float3 diff = gAlbedo * light.strength * NdotL;

        // Blinn-Phong Specular
        float3 H = normalize(L + V);
        float NdotH = saturate(dot(N, H));
        float specIntensity = pow(NdotH, max(gSpecularExponent, 1.0f));
        float3 specularComponent = light.strength * specIntensity * 0.3f;

        // Accumulate
        diffuse += diff * attenuation;
        specular += specularComponent * attenuation;
    }

    float3 finalColor = ambient + diffuse + specular;
    return float4(finalColor, 1.0f);
}