

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

// Sobel算子核（3x3）
static const float kernelX[3][3] =
{ // 水平梯度核（x方向）
    { -1.0, 0.0, 1.0 },
    { -2.0, 0.0, 2.0 },
    { -1.0, 0.0, 1.0 }
};
static const float kernelY[3][3] =
{ // 垂直梯度核（y方向）
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
    
    // 获取纹理尺寸的倒数（texel大小）
    float texelSizeX = 1.0 / SrcWidth;
    float texelSizeY = 1.0 / SrcHeight;
    
    // 初始化梯度和法线分量
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
             // 限制UV在[0,1]范围内（防止越界采样）
            float2 sampleUV = clamp(uv + offset, 0.0, 1.0);
            // 采样高度值
            float height = HeightmapTexture.Sample(sampler_LinearClamp, sampleUV).x;
            // 累加Sobel核的卷积结果
            dx += kernelX[k][l] * height;
            dy += kernelY[k][l] * height;
        }
    }
    
    // 调整法线强度（根据高度图范围调整，通常0.5~2.0）
    float heightScale = 1.0;
    // 计算法线的切线空间分量（x,y为梯度反方向，z为垂直分量）
    float nx = -dx * heightScale;
    float ny = -dy * heightScale;
    float nz = sqrt(saturate(1.0 - nx * nx - ny * ny)); // 防止负数开根号
    
    // 将法线向量从[-1,1]转换到[0,1]
    float3 normal;
    normal.x = nx * 0.5 + 0.5;
    normal.y = nz * 0.5 + 0.5;
    normal.z = ny * 0.5 + 0.5;
    
    RWNormalmapTexture[id.xy] = float4(normal.xyz, 0);
}