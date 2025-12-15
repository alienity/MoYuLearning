

//#include "./TerrainCommonInput.hlsl"
//#include "../../ShaderLibrary/Common.hlsl"

//// https://github.com/wlgys8/GPUDrivenTerrainLearn

// A common method is using a Sobel filter for a weighted/smooth derivative in each direction.
// https://stackoverflow.com/questions/5281261/generating-a-normal-map-from-a-height-map

cbuffer RootConstants : register(b0, space0)
{
    uint heightmapIndex;
    uint normalmapIndex;
};

SamplerState sampler_LinearClamp : register(s10);

// Sobel operator kernel (3x3)
static const float kernelX[3][3] =
{ // Horizontal gradient kernel (x-direction)
    { -1.0, 0.0, 1.0 },
    { -2.0, 0.0, 2.0 },
    { -1.0, 0.0, 1.0 }
};
static const float kernelY[3][3] =
{ // Vertical gradient kernel (y-direction)
    { -1.0, -2.0, -1.0 },
    { 0.0, 0.0, 0.0 },
    { 1.0, 2.0, 1.0 }
};

[numthreads(8, 8, 1)]
void GenerateNormalMap(uint3 id : SV_DispatchThreadID)
{
    Texture2D<float4> HeightmapTexture = ResourceDescriptorHeap[heightmapIndex];
    RWTexture2D<float4> RWNormalmapTexture = ResourceDescriptorHeap[normalmapIndex];

    uint SrcWidth, SrcHeight;
    HeightmapTexture.GetDimensions(SrcWidth, SrcHeight);
    
    float2 uv = (id.xy + 0.5f) / float2(SrcWidth, SrcHeight);
    
    // Get the reciprocal of texture dimensions (texel size)
    float texelSizeX = 1.0 / SrcWidth;
    float texelSizeY = 1.0 / SrcHeight;
    
    // Initialize gradient and normal components
    float dx = 0.0, dy = 0.0;
    
    [unroll]
    for (int k = 0; k < 3; k++)
    {
        [unroll]
        for (int l = 0; l < 3; l++)
        {
            float2 offset = float2(
                (l - 1) * texelSizeX, // 列偏移：-texelSizeX, 0, +texelSizeX
                (k - 1) * texelSizeY // 行偏移：-texelSizeY, 0, +texelSizeY
            );
             // Limit UV within [0,1] range (prevent out-of-bounds sampling)
            float2 sampleUV = clamp(uv + offset, 0.0, 1.0);
            // Sample height value
            float height = HeightmapTexture.Sample(sampler_LinearClamp, sampleUV).x;
            // Accumulate convolution result of Sobel kernel
            dx += kernelX[k][l] * height;
            dy += kernelY[k][l] * height;
        }
    }
    
    // Adjust normal strength (adjusted according to height map range, typically 0.5~2.0)
    float heightScale = 1.0;
    // Calculate tangent space components of normal (x,y are reverse gradient directions, z is vertical component)
    float nx = -dx * heightScale;
    float ny = -dy * heightScale;
    float nz = sqrt(saturate(1.0 - nx * nx - ny * ny)); // Prevent square root of negative numbers
    
    // Convert normal vector from [-1,1] to [0,1]
    float3 normal;
    normal.x = nx * 0.5 + 0.5;
    normal.y = ny * 0.5 + 0.5;
    normal.z = nz * 0.5 + 0.5;
    
    RWNormalmapTexture[id.xy] = float4(normal.xyz, 0);
}