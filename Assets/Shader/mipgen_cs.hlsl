Texture2D<float4> gSourceTexture : register(t0);
RWTexture2D<float4> gDestTexture : register(u0);

SamplerState gLinearClampSampler : register(s0);

cbuffer cbMipGen : register(b0)
{
    uint gSourceMipLevel;
};

[numthreads(8, 8, 1)]
void CS(uint3 dispatchThreadId : SV_DispatchThreadID)
{
    uint dstWidth;
    uint dstHeight;

    gDestTexture.GetDimensions(dstWidth, dstHeight);

    if (dispatchThreadId.x >= dstWidth ||
        dispatchThreadId.y >= dstHeight)
    {
        return;
    }

    float2 uv =
        (float2(dispatchThreadId.xy) + 0.5f) /
        float2(dstWidth, dstHeight);

    float4 color =
        gSourceTexture.SampleLevel(
            gLinearClampSampler,
            uv,
            gSourceMipLevel
        );

    gDestTexture[dispatchThreadId.xy] = color;
}