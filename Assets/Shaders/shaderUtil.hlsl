struct Light
{
    float3  Strength;
    float   FalloffStart;   // point/spot light only
    float3  Direction;      // directional/spot light
    float   FalloffEnd;     // point/spot light only
    float3  Position;       // point light only
    float   SpotPower;      // spot light only
};

struct Material
{
    float4  DiffuseAlbedo;
    float3  FresnelR0;
    float   Shininess; // Shininess = 1 - Roughness
};

float CalcAttenuation(float d, float falloffStart, float falloffEnd)
{
    // Linear falloff attenuation
    return saturate((falloffEnd - d) / (falloffEnd - falloffStart));
}

// Schlick approximation for Fresnel reflectance
float3 SchlickFresnel(float3 R0, float3 normal, float3 lightVec)
{
    float cosIncidentAngle = saturate(dot(normal, lightVec));
    float f0 = 1.0f - cosIncidentAngle;
    float3 reflectPercent = R0 + (1.0f - R0) * pow(f0, 5.0f);
    return reflectPercent;
}

float3 BlinnPhong(float3 lightStrength, float3 lightVec, float3 normal, float3 toEye, Material mat)
{
    // Calculate Blinn-Phong specular with Fresnel-Schlick
    const float m = mat.Shininess * 256.0f;
    float3 halfVec = normalize(toEye + lightVec);

    float NdotH = max(dot(normal, halfVec), 0.0f);
    float roughnessFactor = ((m + 8.0f) * pow(NdotH, m)) / 8.0f;

    float3 fresnelFactor = SchlickFresnel(mat.FresnelR0, halfVec, lightVec);

    // Scale specular to avoid HDR overflow if rendering in LDR
    float3 specAlbedo = fresnelFactor * roughnessFactor;
    specAlbedo = specAlbedo / (specAlbedo + 1.0f);

    return (mat.DiffuseAlbedo.rgb + specAlbedo) * lightStrength;
}

float3 ComputeDirectionalLight(Light L, Material mat, float3 normal, float3 toEye)
{
    // The light vector points opposite to the direction the light rays travel.
    float3 lightVec = -L.Direction;

    // Lambert's cosine law: scale light by the cosine of the angle between light and normal
    float ndotl = max(dot(lightVec, normal), 0.0f);

    // Calculate light strength based on light intensity and ndotl
    float3 lightStrength = L.Strength * ndotl;

    // Calculate final lighting using Blinn-Phong model
    return BlinnPhong(lightStrength, lightVec, normal, toEye, mat);
}

float3 ComputePointLight(Light L, Material mat, float3 pos, float3 normal, float3 toEye)
{
    // Vector from surface point to light position
    float3 lightVec = L.Position - pos;

    // Distance from surface to light
    float d = length(lightVec);

    // Range check: if beyond falloff end, no contribution
    if (d > L.FalloffEnd)
        return float3(0.0f, 0.0f, 0.0f);

    // Normalize light vector
    lightVec /= d;

    // Lambert's cosine law: scale light by cosine of angle between light and surface normal
    float ndotl = max(dot(lightVec, normal), 0.0f);

    // Calculate light strength
    float3 lightStrength = L.Strength * ndotl;

    // Attenuate light based on distance falloff
    float att = CalcAttenuation(d, L.FalloffStart, L.FalloffEnd);
    lightStrength *= att;

    // Apply Blinn-Phong lighting model
    return BlinnPhong(lightStrength, lightVec, normal, toEye, mat);
}

float3 ComputeSpotLight(Light L, Material mat, float3 pos, float3 normal, float3 toEye)
{
    // Vector from surface to light
    float3 lightVec = L.Position - pos;

    // Distance from surface to light
    float d = length(lightVec);

    // Range test: no contribution if beyond falloff end
    if (d > L.FalloffEnd)
        return float3(0.0f, 0.0f, 0.0f);

    // Normalize light vector
    lightVec /= d;

    // Lambert's cosine law: dot product of normal and light direction
    float ndotl = max(dot(lightVec, normal), 0.0f);

    // Calculate light strength
    float3 lightStrength = L.Strength * ndotl;

    // Attenuate by distance
    float att = CalcAttenuation(d, L.FalloffStart, L.FalloffEnd);
    lightStrength *= att;

    // Spotlight effect: scale by spotlight cone factor
    float spotFactor = pow(max(dot(-lightVec, L.Direction), 0.0f), L.SpotPower);
    lightStrength *= spotFactor;

    // Calculate final lighting with Blinn-Phong model
    return BlinnPhong(lightStrength, lightVec, normal, toEye, mat);
}