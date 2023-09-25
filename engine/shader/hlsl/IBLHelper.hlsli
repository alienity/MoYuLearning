#include "CommonMath.hlsli"
#include "ColorSpaceUtility.hlsli"

#define IBL_INTEGRATION_IMPORTANCE_SAMPLING_COUNT 64

// https://google.github.io/filament/Filament.html#annex/importancesamplingfortheibl

float2 hammersley(uint i, float numSamples)
{
    uint bits = i;
    bits = (bits << 16) | (bits >> 16);
    bits = ((bits & 0x55555555) << 1) | ((bits & 0xAAAAAAAA) >> 1);
    bits = ((bits & 0x33333333) << 2) | ((bits & 0xCCCCCCCC) >> 2);
    bits = ((bits & 0x0F0F0F0F) << 4) | ((bits & 0xF0F0F0F0) >> 4);
    bits = ((bits & 0x00FF00FF) << 8) | ((bits & 0xFF00FF00) >> 8);
    return float2(i / numSamples, bits / exp2(32));
}

float3 hemisphereImportanceSampleDggx(float2 u, float a) 
{ // pdf = D(a) * cosTheta
    const float phi = 2.0f * (float) F_PI * u.x;
    // NOTE: (aa-1) == (a-1)(a+1) produces better fp accuracy
    const float cosTheta2 = (1 - u.y) / (1 + (a + 1) * ((a - 1) * u.y));
    const float cosTheta = sqrt(cosTheta2);
    const float sinTheta = sqrt(1 - cosTheta2);
    return float3(sinTheta * cos(phi), sinTheta * sin(phi), cosTheta);
}

float3 hemisphereCosSample(float2 u)
{  // pdf = cosTheta / F_PI;
    const float phi = 2.0f * (float) F_PI * u.x;
    const float cosTheta2 = 1 - u.y;
    const float cosTheta = sqrt(cosTheta2);
    const float sinTheta = sqrt(1 - cosTheta2);
    return float3(sinTheta * cos(phi), sinTheta * sin(phi), cosTheta);
}

float3 hemisphereUniformSample(float2 u)
{ // pdf = 1.0 / (2.0 * F_PI);
    const float phi = 2.0f * (float) F_PI * u.x;
    const float cosTheta = 1 - u.y;
    const float sinTheta = sqrt(1 - cosTheta * cosTheta);
    return float3(sinTheta * cos(phi), sinTheta * sin(phi), cosTheta);
}

float Visibility(float NoV, float NoL, float a)
{
    // Heitz 2014, "Understanding the Masking-Shadowing Function in Microfacet-Based BRDFs"
    // Height-correlated GGX
    float a2 = a * a;
    float GGXL = NoV * sqrt((-NoL * a2 + NoL) * NoL + a2);
    float GGXV = NoL * sqrt((-NoV * a2 + NoV) * NoV + a2);
    return 0.5f / (GGXV + GGXL);
}

/*
 *
 * Importance sampling GGX - Trowbridge-Reitz
 * ------------------------------------------
 *
 * Important samples are chosen to integrate Dggx() * cos(theta) over the hemisphere.
 *
 * All calculations are made in tangent space, with n = [0 0 1]
 *
 *                      h (important sample)
 *                     /.
 *                    / .
 *                   /  .
 *                  /   .
 *         --------o----+-------> n
 *                   cos(theta)
 *                    = n•h
 *
 *  h is micro facet's normal
 *  l is the reflection of v around h, l = reflect(-v, h)  ==>  v•h = l•h
 *
 *  n•v is given as an input parameter at runtime
 *
 *  Since n = [0 0 1], we also have v.z = n•v
 *
 *  Since we need to compute v•h, we chose v as below. This choice only affects the
 *  computation of v•h (and therefore the fresnel term too), but doesn't affect
 *  n•l, which only relies on l.z (which itself only relies on v.z, i.e.: n•v)
 *
 *      | sqrt(1 - (n•v)^2)     (sin)
 *  v = | 0
 *      | n•v                   (cos)
 *
 *
 *  h = important_sample_ggx()
 *
 *  l = reflect(-v, h) = 2 * v•h * h - v;
 *
 *  n•l = [0 0 1] • l = l.z
 *
 *  n•h = [0 0 1] • l = h.z
 *
 *
 *  pdf() = D(h) <n•h> |J(h)|
 *
 *               1
 *  |J(h)| = ----------
 *            4 <v•h>
 *
 *
 * Evaluating the integral
 * -----------------------
 *
 * We are trying to evaluate the following integral:
 *
 *                    /
 *             Er() = | fr(s) <n•l> ds
 *                    /
 *                    Ω
 *
 * For this, we're using importance sampling:
 *
 *                    1     fr(h)
 *            Er() = --- ∑ ------- <n•l>
 *                    N  h   pdf
 *
 * with:
 *
 *            fr() = D(h) F(h) V(v, l)
 *
 *
 *  It results that:
 *
 *            1                        4 <v•h>
 *    Er() = --- ∑ D(h) F(h) V(v, l) ------------ <n•l>
 *            N  h                     D(h) <n•h>
 *
 *
 *  +-------------------------------------------+
 *  |          4                  <v•h>         |
 *  |  Er() = --- ∑ F(h) V(v, l) ------- <n•l>  |
 *  |          N  h               <n•h>         |
 *  +-------------------------------------------+
 *
 */

float2 DFV(float NoV, float linearRoughness, uint numSamples)
{
    float2 r = 0.0f;
    float3 V = float3(sqrt(1.0f - NoV * NoV), 0, NoV);
    for (uint i = 0; i < numSamples; i++)
    {
        float2 Xi = hammersley(i, numSamples);
        float3 H = hemisphereImportanceSampleDggx(Xi, linearRoughness);
        float3 L = 2.0f * dot(V, H) * H - V;

        float VoH = saturate(dot(V, H));
        float NoL = saturate(L.z);
        float NoH = saturate(H.z);

        if (NoL > 0.0f) {
            /*
            * Fc = (1 - V•H)^5
            * F(h) = f0*(1 - Fc) + f90*Fc
            *
            * f0 and f90 are known at runtime, but thankfully can be factored out, allowing us
            * to split the integral in two terms and store both terms separately in a LUT.
            *
            * At runtime, we can reconstruct Er() exactly as below:
            *
            *            4                      <v•h>
            *   DFV.x = --- ∑ (1 - Fc) V(v, l) ------- <n•l>
            *            N  h                   <n•h>
            *
            *
            *            4                      <v•h>
            *   DFV.y = --- ∑ (    Fc) V(v, l) ------- <n•l>
            *            N  h                   <n•h>
            *
            *
            *   Er() = f0 * DFV.x + f90 * DFV.y
            *
            */
            float Gv = Visibility(NoV, NoL, linearRoughness) * NoL * (VoH / NoH);
            float Fc = pow5(1 - VoH);
            r.x += Gv * (1 - Fc);
            r.y += Gv * Fc;
        }
    }
    return r * (4.0f / numSamples);
}

float2 DFV_Multiscatter(float NoV, float linearRoughness, uint numSamples)
{
    float2 r = 0;
    const float3 V = float3(sqrt(1 - NoV * NoV), 0, NoV);
    for (uint i = 0; i < numSamples; i++)
    {
        const float2 u = hammersley(i, numSamples);
        const float3 H = hemisphereImportanceSampleDggx(u, linearRoughness);
        const float3 L = 2 * dot(V, H) * H - V;
        
        const float VoH = saturate(dot(V, H));
        const float NoL = saturate(L.z);
        const float NoH = saturate(H.z);
        
        if (NoL > 0) {
            const float v = Visibility(NoV, NoL, linearRoughness) * NoL * (VoH / NoH);
            const float Fc = pow5(1 - VoH);
            /*
             * Assuming f90 = 1
             *   Fc = (1 - V•H)^5
             *   F(h) = f0*(1 - Fc) + Fc
             *
             * f0 and f90 are known at runtime, but thankfully can be factored out, allowing us
             * to split the integral in two terms and store both terms separately in a LUT.
             *
             * At runtime, we can reconstruct Er() exactly as below:
             *
             *            4                <v•h>
             *   DFV.x = --- ∑ Fc V(v, l) ------- <n•l>
             *            N  h             <n•h>
             *
             *
             *            4                <v•h>
             *   DFV.y = --- ∑    V(v, l) ------- <n•l>
             *            N  h             <n•h>
             *
             *
             *   Er() = (1 - f0) * DFV.x + f0 * DFV.y
             *
             *        = mix(DFV.xxx, DFV.yyy, f0)
             *
             */
            r.x += v * Fc;
            r.y += v;
        }
    }
    return r * (4.0f / numSamples);
}

//! Computes the "DFG" term of the "split-sum" approximation and stores it in a 2D image
float3 DFG(int2 dstSize, int2 inTexPos, bool multiscatter)
{
    const int width = dstSize.x;
    const int height = dstSize.y;
    
    const float h = (float) height;
    const float coord = saturate((h - inTexPos.y + 0.5f) / h);
    // map the coordinate in the texture to a linear_roughness,
    // here we're using ^2, but other mappings are possible.
    // ==> coord = sqrt(linear_roughness)
    const float linear_roughness = coord * coord;
    const float NoV = saturate((inTexPos.x + 0.5f) / width);
    
    float3 r;
    if (multiscatter)
        r = float3(DFV_Multiscatter(NoV, linear_roughness, 1024), 0);
    else
        r = float3(DFV(NoV, linear_roughness, 1024), 0);
    
    return r;
}



// pre-filter the environment, based on some input roughness that varies over each mipmap level 
// of the pre-filter cubemap (from 0.0 to 1.0), and store the result in prefilteredColor.
// 所以这里用一次Draw一个Cube来实现是最方便的，因为光栅化会自动对Cube的每个像素都进行处理
// 针对每个lod等级，都做一次绘制，每个lod等级使用的roughness不一样，
// lod 0 --- α 0
// lod 1 --- α 0.2
// lod 2 --- α 0.4
// lod 3 --- α 0.6
// lod 4 --- α 0.8
/*
 *
 * Importance sampling GGX - Trowbridge-Reitz
 * ------------------------------------------
 *
 * Important samples are chosen to integrate Dggx() * cos(theta) over the hemisphere.
 *
 * All calculations are made in tangent space, with n = [0 0 1]
 *
 *             l        h (important sample)
 *             .\      /.
 *             . \    / .
 *             .  \  /  .
 *             .   \/   .
 *         ----+---o----+-------> n [0 0 1]
 *     cos(2*theta)     cos(theta)
 *        = n•l            = n•h
 *
 *  v = n
 *  f0 = f90 = 1
 *  V = 1
 *
 *  h is micro facet's normal
 *
 *  l is the reflection of v (i.e.: n) around h  ==>  n•h = l•h = v•h
 *
 *  h = important_sample_ggx()
 *
 *  n•h = [0 0 1]•h = h.z
 *
 *  l = reflect(-n, h)
 *    = 2 * (n•h) * h - n;
 *
 *  n•l = cos(2 * theta)
 *      = cos(theta)^2 - sin(theta)^2
 *      = (n•h)^2 - (1 - (n•h)^2)
 *      = 2(n•h)^2 - 1
 *
 *
 *  pdf() = D(h) <n•h> |J(h)|
 *
 *               1
 *  |J(h)| = ----------
 *            4 <v•h>
 *
 *
 * Pre-filtered importance sampling
 * --------------------------------
 *
 *  see: "Real-time Shading with Filtered Importance Sampling", Jaroslav Krivanek
 *  see: "GPU-Based Importance Sampling, GPU Gems 3", Mark Colbert
 *
 *
 *                   Ωs
 *     lod = log4(K ----)
 *                   Ωp
 *
 *     log4(K) = 1, works well for box filters
 *     K = 4
 *
 *             1
 *     Ωs = ---------, solid-angle of an important sample
 *           N * pdf
 *
 *              4 PI
 *     Ωp ~ --------------, solid-angle of a sample in the base cubemap
 *           texel_count
 *
 *
 * Evaluating the integral
 * -----------------------
 *
 *                    K     fr(h)
 *            Er() = --- ∑ ------- L(h) <n•l>
 *                    N  h   pdf
 *
 * with:
 *
 *            fr() = D(h)
 *
 *                       N
 *            K = -----------------
 *                    fr(h)
 *                 ∑ ------- <n•l>
 *                 h   pdf
 *
 *
 *  It results that:
 *
 *            K           4 <v•h>
 *    Er() = --- ∑ D(h) ------------ L(h) <n•l>
 *            N  h        D(h) <n•h>
 *
 *
 *              K
 *    Er() = 4 --- ∑ L(h) <n•l>
 *              N  h
 *
 *                  N       4
 *    Er() = ------------- --- ∑ L(v) <n•l>
 *             4 ∑ <n•l>    N
 *
 *
 *  +------------------------------+
 *  |          ∑ <n•l> L(h)        |
 *  |  Er() = --------------       |
 *  |            ∑ <n•l>           |
 *  +------------------------------+
 *
 */

float4 roughnessFilter(TextureCube<float4> envMap, SamplerState enMapSampler, int maxNumSamples, float3 direction, float linearRoughness)
{
    float3 N = normalize(direction);
    
    const float3 upVector = abs(N.z) < 0.999 ? float3(0, 0, 1) : float3(1, 0, 0);
    const float3 tangentX = normalize(cross(upVector, N));
    const float3 tangentY = cross(N, tangentX);
    float3x3 tangent2World = transpose(float3x3(tangentX, tangentY, N));

    float weight = 0;
    // index of the sample to use
    // our goal is to use maxNumSamples for which NoL is > 0
    // to achieve this, we might have to try more samples than
    // maxNumSamples
    float3 prefilteredColor = float3(0.0, 0.0, 0.0);     
    for (uint sampleIndex = 0u; sampleIndex < maxNumSamples; ++sampleIndex)
    {
        // get Hammersley distribution for the half-sphere
        float2 Xi = hammersley(sampleIndex, maxNumSamples);
        // Importance sampling GGX - Trowbridge-Reitz
        float3 H = hemisphereImportanceSampleDggx(Xi, linearRoughness);
        
        const float NoH = H.z;
        const float NoH2 = H.z * H.z;
        const float NoL = 2 * NoH2 - 1;
        float3 L = float3(2 * NoH * H.x, 2 * NoH * H.y, NoL);

        L = mul(tangent2World, L);
        
        if (NoL > 0.0)
        {
            prefilteredColor += clamp(envMap.Sample(enMapSampler, L).rgb, 0, 30) * NoL;
            weight += NoL;
        }
    }
    prefilteredColor = prefilteredColor / weight;

    return float4(prefilteredColor, 1.0);
}


/*
 *
 * Importance sampling
 * -------------------
 *
 * Important samples are chosen to integrate cos(theta) over the hemisphere.
 *
 * All calculations are made in tangent space, with n = [0 0 1]
 *
 *                      l (important sample)
 *                     /.
 *                    / .
 *                   /  .
 *                  /   .
 *         --------o----+-------> n (direction)
 *                   cos(theta)
 *                    = n•l
 *
 *
 *  'direction' is given as an input parameter, and serves as tge z direction of the tangent space.
 *
 *  l = important_sample_cos()
 *
 *  n•l = [0 0 1] • l = l.z
 *
 *           n•l
 *  pdf() = -----
 *           PI
 *
 *
 * Pre-filtered importance sampling
 * --------------------------------
 *
 *  see: "Real-time Shading with Filtered Importance Sampling", Jaroslav Krivanek
 *  see: "GPU-Based Importance Sampling, GPU Gems 3", Mark Colbert
 *
 *
 *                   Ωs
 *     lod = log4(K ----)
 *                   Ωp
 *
 *     log4(K) = 1, works well for box filters
 *     K = 4
 *
 *             1
 *     Ωs = ---------, solid-angle of an important sample
 *           N * pdf
 *
 *              4 PI
 *     Ωp ~ --------------, solid-angle of a sample in the base cubemap
 *           texel_count
 *
 *
 * Evaluating the integral
 * -----------------------
 *
 * We are trying to evaluate the following integral:
 *
 *                     /
 *             Ed() =  | L(s) <n•l> ds
 *                     /
 *                     Ω
 *
 * For this, we're using importance sampling:
 *
 *                    1     L(l)
 *            Ed() = --- ∑ ------- <n•l>
 *                    N  l   pdf
 *
 *
 *  It results that:
 *
 *             1           PI
 *    Ed() = ---- ∑ L(l) ------  <n•l>
 *            N   l        n•l
 *
 *
 *  To avoid multiplying by 1/PI in the shader, we do it here, which simplifies to:
 *
 *  +----------------------+
 *  |          1           |
 *  |  Ed() = ---- ∑ L(l)  |
 *  |          N   l       |
 *  +----------------------+
 *
 */

float3 diffuseIrradiance(TextureCube<float4> envMap, SamplerState enMapSampler, int maxNumSamples, float3 direction)
{
    float3 N = normalize(direction);
    
    float3 upVector = abs(N.z) < 0.999 ? float3(0, 0, 1) : float3(1, 0, 0);
    float3 tangentX = normalize(cross(upVector, N));
    float3 tangentY = cross(N, tangentX);
    float3x3 tangent2World = transpose(float3x3(tangentX, tangentY, N));
    
    float3 prefilteredColor = float3(0.0, 0.0, 0.0);
    for (uint sampleIndex = 0u; sampleIndex < maxNumSamples; ++sampleIndex)
    {
        // get Hammersley distribution for the half-sphere
        float2 u = hammersley(sampleIndex, maxNumSamples);
        float3 L = hemisphereCosSample(u);
        float3 N = float3(0, 0, 1);
        float NoL = dot(N, L);
        
        L = mul(tangent2World, L);
        
        if (NoL > 0.0)
        {
            float3 envColor = clamp(envMap.Sample(enMapSampler, L).rgb, 0, 30);
            prefilteredColor += envColor;
        }
    }
    
    return prefilteredColor / maxNumSamples;

}

// 在d3d12中写入到指定mip中，当然如果直接用ComputeShader去做计算，就可以一次性计算出所有的mip，那我还是用cS做吧
// https://www.gamedev.net/forums/topic/688777-render-to-specific-cubemap-mip/5343021/

//*****************************************************************
// SH
//*****************************************************************

/*
 * returns n! / d!
 */
uint SHindex(uint m, uint l)
{
    return l * (l + 1) + m;
}

/*
 * Calculates non-normalized SH bases, i.e.:
 *  m > 0, cos(m*phi)   * P(m,l)
 *  m < 0, sin(|m|*phi) * P(|m|,l)
 *  m = 0, P(0,l)
 */
void computeShBasisBand2(out float SHb[9], const float3 s)
{
    /*
     * TODO: all the Legendre computation below is identical for all faces, so it
     * might make sense to pre-compute it once. Also note that there is
     * a fair amount of symmetry within a face (which we could take advantage of
     * to reduce the pre-compute table).
     */

    /*
     * Below, we compute the associated Legendre polynomials using recursion.
     * see: http://mathworld.wolfram.com/AssociatedLegendrePolynomial.html
     *
     * Note [0]: s.z == cos(theta) ==> we only need to compute P(s.z)
     *
     * Note [1]: We in fact compute P(s.z) / sin(theta)^|m|, by removing
     * the "sqrt(1 - s.z*s.z)" [i.e.: sin(theta)] factor from the recursion.
     * This is later corrected in the ( cos(m*phi), sin(m*phi) ) recursion.
     */

    // s = (x, y, z) = (sin(theta)*cos(phi), sin(theta)*sin(phi), cos(theta))

    uint numBands = 2;

    // handle m=0 separately, since it produces only one coefficient
    float Pml_2 = 0;
    float Pml_1 = 1;
    SHb[0] =  Pml_1;
    for (uint l=1; l<numBands; l++) {
        float Pml = ((2*l-1.0f)*Pml_1*s.z - (l-1.0f)*Pml_2) / l;
        Pml_2 = Pml_1;
        Pml_1 = Pml;
        SHb[SHindex(0, l)] = Pml;
    }
    float Pmm = 1;
    for (uint m=1 ; m<numBands ; m++) {
        Pmm = (1.0f - 2*m) * Pmm;      // See [1], divide by sqrt(1 - s.z*s.z);
        Pml_2 = Pmm;
        Pml_1 = (2*m + 1.0f)*Pmm*s.z;
        // l == m
        SHb[SHindex(-m, m)] = Pml_2;
        SHb[SHindex( m, m)] = Pml_2;
        if (m+1 < numBands) {
            // l == m+1
            SHb[SHindex(-m, m+1)] = Pml_1;
            SHb[SHindex( m, m+1)] = Pml_1;
            for (uint l=m+2 ; l<numBands ; l++) {
                float Pml = ((2*l - 1.0f)*Pml_1*s.z - (l + m - 1.0f)*Pml_2) / (l-m);
                Pml_2 = Pml_1;
                Pml_1 = Pml;
                SHb[SHindex(-m, l)] = Pml;
                SHb[SHindex( m, l)] = Pml;
            }
        }
    }

    // At this point, SHb contains the associated Legendre polynomials divided
    // by sin(theta)^|m|. Below we compute the SH basis.
    //
    // ( cos(m*phi), sin(m*phi) ) recursion:
    // cos(m*phi + phi) == cos(m*phi)*cos(phi) - sin(m*phi)*sin(phi)
    // sin(m*phi + phi) == sin(m*phi)*cos(phi) + cos(m*phi)*sin(phi)
    // cos[m+1] == cos[m]*s.x - sin[m]*s.y
    // sin[m+1] == sin[m]*s.x + cos[m]*s.y
    //
    // Note that (d.x, d.y) == (cos(phi), sin(phi)) * sin(theta), so the
    // code below actually evaluates:
    //      (cos((m*phi), sin(m*phi)) * sin(theta)^|m|
    float Cm = s.x;
    float Sm = s.y;
    for (uint m = 1; m <= numBands; m++) {
        for (uint l = m; l < numBands; l++) {
            SHb[SHindex(-m, l)] *= Sm;
            SHb[SHindex( m, l)] *= Cm;
        }
        float Cm1 = Cm * s.x - Sm * s.y;
        float Sm1 = Sm * s.x + Cm * s.y;
        Cm = Cm1;
        Sm = Sm1;
    }
}

/*
 * returns n! / d!
 */
float factorial(uint n, uint d = 1)
{
   d = max(uint(1), d);
   n = max(uint(1), n);
   float r = 1.0;
   if (n == d) {
       // intentionally left blank
   } else if (n > d) {
       for ( ; n>d ; n--) {
           r *= n;
       }
   } else {
       for ( ; d>n ; d--) {
           r *= d;
       }
       r = 1.0f / r;
   }
   return r;
}

/*
 * SH scaling factors:
 *  returns sqrt((2*l + 1) / 4*pi) * sqrt( (l-|m|)! / (l+|m|)! )
 */
float Kml(uint m, uint l)
{
    m = m < 0 ? -m : m;  // abs() is not constexpr
    const float K = (2 * l + 1) * factorial(uint(l - m), uint(l + m));
    return sqrt(K) * (F_2_SQRTPI * 0.25);
}

void KiBand2(float ki[4])
{
    uint numBands = 2;
    const uint numCoefs = numBands * numBands;
    float K[numCoefs];

    for (uint l = 0; l < numBands; l++) {
        K[SHindex(0, l)] = Kml(0, l);
        for (uint m = 1; m <= l; m++) {
            K[SHindex(m, l)] =
            K[SHindex(-m, l)] = F_SQRT2 * Kml(m, l);
        }
    }
    return K;
}

// < cos(theta) > SH coefficients pre-multiplied by 1 / K(0,l)
float computeTruncatedCosSh(uint l)
{
    if (l == 0) {
        return F_PI;
    } else if (l == 1) {
        return 2 * F_PI / 3;
    } else if (l & 1u) {
        return 0;
    }
    const uint l_2 = l / 2;
    float A0 = ((l_2 & 1u) ? 1.0f : -1.0f) / ((l + 2) * (l - 1));
    float A1 = factorial(l, l_2) / (factorial(l_2) * (1 << l));
    return 2 * F_PI * A0 * A1;
}

/*
 * This computes the 3-bands SH coefficients of the Cubemap convoluted by the
 * truncated cos(theta) (i.e.: saturate(s.z)), pre-scaled by the reconstruction
 * factors.
 */
void preprocessSHForShader(inout float3 SH[2])
{
    constexpr size_t numBands = 3;
    constexpr size_t numCoefs = numBands * numBands;

    // Coefficient for the polynomial form of the SH functions -- these were taken from
    // "Stupid Spherical Harmonics (SH)" by Peter-Pike Sloan
    // They simply come for expanding the computation of each SH function.
    //
    // To render spherical harmonics we can use the polynomial form, like this:
    //          c += sh[0] * A[0];
    //          c += sh[1] * A[1] * s.y;
    //          c += sh[2] * A[2] * s.z;
    //          c += sh[3] * A[3] * s.x;
    //          c += sh[4] * A[4] * s.y * s.x;
    //          c += sh[5] * A[5] * s.y * s.z;
    //          c += sh[6] * A[6] * (3 * s.z * s.z - 1);
    //          c += sh[7] * A[7] * s.z * s.x;
    //          c += sh[8] * A[8] * (s.x * s.x - s.y * s.y);
    //
    // To save math in the shader, we pre-multiply our SH coefficient by the A[i] factors.
    // Additionally, we include the lambertian diffuse BRDF 1/pi.

    constexpr float M_SQRT_PI = 1.7724538509f;
    constexpr float M_SQRT_3  = 1.7320508076f;
    constexpr float M_SQRT_5  = 2.2360679775f;
    constexpr float M_SQRT_15 = 3.8729833462f;
    constexpr float A[numCoefs] = {
                  1.0f / (2.0f * M_SQRT_PI),    // 0  0
            -M_SQRT_3  / (2.0f * M_SQRT_PI),    // 1 -1
             M_SQRT_3  / (2.0f * M_SQRT_PI),    // 1  0
            -M_SQRT_3  / (2.0f * M_SQRT_PI),    // 1  1
             M_SQRT_15 / (2.0f * M_SQRT_PI),    // 2 -2
            -M_SQRT_15 / (2.0f * M_SQRT_PI),    // 3 -1
             M_SQRT_5  / (4.0f * M_SQRT_PI),    // 3  0
            -M_SQRT_15 / (2.0f * M_SQRT_PI),    // 3  1
             M_SQRT_15 / (4.0f * M_SQRT_PI)     // 3  2
    };

    for (uint i = 0; i < numCoefs; i++) {
        SH[i] *= A[i] * F_1_PI;
    }
}
