#ifndef MATERIAL_PROPERTIES_HLSL
#define MATERIAL_PROPERTIES_HLSL

#ifdef _CPP_MACRO_
#define PREMACRO(T) glm::T
#else
#define PREMACRO(T) T
#endif

struct PropertiesPerMaterial
{
    PREMACRO(uint) _EmissiveColorMapIndex;
    PREMACRO(uint) _BaseColorMapIndex;
    PREMACRO(uint) _MaskMapIndex; // MaskMap is RGBA: Metallic, Ambient Occlusion (Optional), detail Mask (Optional), Smoothness
    PREMACRO(uint) _BentNormalMapIndex;
    
    PREMACRO(uint) _BentNormalMapOSIndex;
    PREMACRO(uint) _NormalMapIndex; // Tangent space normal map
    PREMACRO(uint) _NormalMapOSIndex; // Object space normal map - no good default value    
    PREMACRO(uint) _DetailMapIndex;
    
    PREMACRO(uint) _HeightMapIndex;
    PREMACRO(uint) _TangentMapIndex;
    PREMACRO(uint) _TangentMapOSIndex;    
    PREMACRO(uint) _AnisotropyMapIndex;
    
    PREMACRO(uint) _SubsurfaceMaskMapIndex;
    PREMACRO(uint) _TransmissionMaskMapIndex;
    PREMACRO(uint) _ThicknessMapIndex;
    PREMACRO(uint) _IridescenceThicknessMapIndex;
    
    PREMACRO(uint) _IridescenceMaskMapIndex;
    PREMACRO(uint) _SpecularColorMapIndex;
    PREMACRO(uint) _TransmittanceColorMapIndex;
    PREMACRO(uint) _CoatMaskMapIndex;
    
    // shared constant between lit and layered lit
    float _AlphaCutoff;
    float _UseShadowThreshold;
    float _AlphaCutoffShadow;
    float _AlphaCutoffPrepass;
    
    float _AlphaCutoffPostpass;
    float _BlendMode;
    float _EnableBlendModePreserveSpecularLighting;
    float _PPDMaxSamples;
    
    float _PPDMinSamples;
    float _PPDLodThreshold;
    float _PPDPrimitiveLength;
    float _PPDPrimitiveWidth;
    
    PREMACRO(float4) _InvPrimScale; // Only XY are used
    
    PREMACRO(float3) _EmissiveColor;
    float _Padding01;

    float _AlbedoAffectEmissive;
    float _EmissiveExposureWeight;
    int _SpecularOcclusionMode;
    // Transparency
    float _Ior;
    
    PREMACRO(float3) _TransmittanceColor;
    float _ATDistance;
    
    // Set of users variables
    PREMACRO(float4) _BaseColor;

    float _Metallic;
    float _MetallicRemapMin;
    float _MetallicRemapMax;
    float _Smoothness;
    
    float _SmoothnessRemapMin;
    float _SmoothnessRemapMax;
    float _AlphaRemapMin;
    float _AlphaRemapMax;
    
    float _AORemapMin;
    float _AORemapMax;
    float _NormalScale;    
    float _DetailAlbedoScale;
    
    float _DetailNormalScale;
    float _DetailSmoothnessScale;
    float _HeightAmplitude;
    float _HeightCenter;
    
    float _Anisotropy;
    float _SubsurfaceMask;
    float _TransmissionMask;    
    float _Thickness;
    
    PREMACRO(float4) _ThicknessRemap;
    PREMACRO(float4) _IridescenceThicknessRemap;
    
    float _SurfaceType;
    float3 _Padding03;
    
    float _IridescenceThickness;
    float _IridescenceMask;
    float _CoatMask;
    float _EnergyConservingSpecularColor;
    
    PREMACRO(float4) _SpecularColor;
    PREMACRO(float4) _UVMappingMask;
    PREMACRO(float4) _UVDetailsMappingMask;
    
    float _TexWorldScale;
    float _InvTilingScale;
    float _ObjectSpaceUVMapping;
    float _Padding6;
};

#endif
