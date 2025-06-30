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

#define MaxLights 16

cbuffer cbPass : register(b2)
{
    // Assuming these macros are defined elsewhere:
    // #define NUM_DIR_LIGHTS ...
    // #define NUM_POINT_LIGHTS ...
    // #define NUM_SPOT_LIGHTS ...
    
    Light gLights[MaxLights];
};

float4 ComputeLighting(Light gLights[MaxLights],
                       Material mat,
                       float3 pos, float3 normal, float3 toEye,
                       float3 shadowFactor)
{
    float3 result = float3(0.0f, 0.0f, 0.0f);
    int i = 0;

#if (NUM_DIR_LIGHTS > 0)
    for (i = 0; i < NUM_DIR_LIGHTS; ++i)
    {
        // Shadow factor assumed to be an array indexed by light
        result += shadowFactor[i] * ComputeDirectionalLight(gLights[i], mat, normal, toEye);
    }
#endif

#if (NUM_POINT_LIGHTS > 0)
    for (i = NUM_DIR_LIGHTS; i < NUM_DIR_LIGHTS + NUM_POINT_LIGHTS; ++i)
    {
        result += ComputePointLight(gLights[i], mat, pos, normal, toEye);
    }
#endif

#if (NUM_SPOT_LIGHTS > 0)
    for (i = NUM_DIR_LIGHTS + NUM_POINT_LIGHTS; 
         i < NUM_DIR_LIGHTS + NUM_POINT_LIGHTS + NUM_SPOT_LIGHTS; ++i)
    {
        result += ComputeSpotLight(gLights[i], mat, pos, normal, toEye);
    }
#endif

    return float4(result, 0.0f);
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
    vout.normalW = normalize(mul(vin.normal, (float3x3)gWorld));

    return vout;
}

// === Pixel Shader ===
float4 PS(sVertexOut pin) : SV_Target
{
    float3 N = normalize(pin.normalW);              // Surface normal at the pixel
    float3 L = normalize(-gLightDirW);              // Vector from pixel to light
    float3 V = normalize(gEyePosW - pin.posW);      // Vector from pixel to eye
    float3 H = normalize(L + V);                    // Half vector = normalize(L + V)

    // Ambient
    float3 ambient = gAlbedo * 0.3f;

    // Diffuse (Lambert)
    float NdotL = saturate(dot(N, L));
    float3 diffuse = gAlbedo * gLightColor * gLightIntensity * NdotL;

    // Specular (Blinn-Phong)
    float NdotH = saturate(dot(N, H));
    float spec = pow(NdotH, max(gSpecularExponent, 1.0f));
    float3 specular = gLightColor * gLightIntensity * spec * 0.3f;

    float3 finalColor = ambient + diffuse + specular;
    return float4(finalColor, 1.0f);
}
