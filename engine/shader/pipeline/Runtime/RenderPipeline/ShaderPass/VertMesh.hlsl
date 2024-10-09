struct VaryingsToPS
{
    VaryingsMeshToPS vmesh;
#ifdef VARYINGS_NEED_PASS
    VaryingsPassToPS vpass;
#endif
};

struct PackedVaryingsToPS
{
#ifdef VARYINGS_NEED_PASS
    PackedVaryingsPassToPS vpass;
#endif
    PackedVaryingsMeshToPS vmesh;
};

PackedVaryingsToPS PackVaryingsToPS(VaryingsToPS input)
{
    PackedVaryingsToPS output;
    output.vmesh = PackVaryingsMeshToPS(input.vmesh);
#ifdef VARYINGS_NEED_PASS
    output.vpass = PackVaryingsPassToPS(input.vpass);
#endif

    return output;
}

FragInputs UnpackVaryingsToFragInputs(PackedVaryingsToPS packedInput)
{
    FragInputs input = UnpackVaryingsMeshToFragInputs(packedInput.vmesh);

#if defined(PLATFORM_SUPPORTS_PRIMITIVE_ID_IN_PIXEL_SHADER) && SHADER_STAGE_FRAGMENT
#if (defined(VARYINGS_NEED_PRIMITIVEID) || (SHADERPASS == SHADERPASS_FULL_SCREEN_DEBUG))
    input.primitiveID = packedInput.primitiveID;
#endif
#endif

#if defined(VARYINGS_NEED_CULLFACE) && SHADER_STAGE_FRAGMENT
    input.isFrontFace = IS_FRONT_VFACE(packedInput.cullFace, true, false);
#endif

    return input;
}

#define VaryingsType VaryingsToPS
#define VaryingsMeshType VaryingsMeshToPS
#define PackedVaryingsType PackedVaryingsToPS
#define PackVaryingsType PackVaryingsToPS

// TODO: Here we will also have all the vertex deformation (GPU skinning, vertex animation, morph target...) or we will need to generate a compute shaders instead (better! but require work to deal with unpacking like fp16)
VaryingsMeshType VertMesh(FrameUniforms frameUniform, RenderDataPerDraw renderData, AttributesMesh input, float3 worldSpaceOffset)
{
    VaryingsMeshType output;
#if defined(USE_CUSTOMINTERP_SUBSTRUCT)
    ZERO_INITIALIZE(VaryingsMeshType, output); // Only required with custom interpolator to quiet the shader compiler about not fully initialized struct
#endif

#ifdef HAVE_MESH_MODIFICATION
    input = ApplyMeshModification(input, _TimeParameters.xyz
    #ifdef USE_CUSTOMINTERP_SUBSTRUCT
        // If custom interpolators are in use, we need to write them to the shader graph generated VaryingsMesh
        , output
    #endif
    );
#endif

    // This return the camera relative position (if enable)
    float3 positionRWS = TransformObjectToWorld(renderData, input.positionOS) + worldSpaceOffset;
#ifdef ATTRIBUTES_NEED_NORMAL
    float3 normalWS = TransformObjectToWorldNormal(renderData, input.normalOS);
#else
    float3 normalWS = float3(0.0, 0.0, 0.0); // We need this case to be able to compile ApplyVertexModification that doesn't use normal.
#endif

#ifdef ATTRIBUTES_NEED_TANGENT
    float4 tangentWS = float4(TransformObjectToWorldDir(renderData, input.tangentOS.xyz), input.tangentOS.w);
#endif

    // Do vertex modification in camera relative space (if enable)
#if defined(HAVE_VERTEX_MODIFICATION)
    ApplyVertexModification(input, normalWS, positionRWS, _TimeParameters.xyz);
#endif

#ifdef VARYINGS_NEED_POSITION_WS
    output.positionRWS = positionRWS;
#endif
#ifdef VARYINGS_NEED_POSITIONPREDISPLACEMENT_WS
    output.positionPredisplacementRWS = positionRWS;
#endif

    output.positionCS = TransformWorldToHClip(frameUniform, positionRWS);
#ifdef VARYINGS_NEED_TANGENT_TO_WORLD
    output.normalWS = normalWS;
    output.tangentWS = tangentWS;
#endif
#if !defined(SHADER_API_METAL) && defined(SHADERPASS) && (SHADERPASS == SHADERPASS_FULL_SCREEN_DEBUG)
    if (_DebugFullScreenMode == FULLSCREENDEBUGMODE_VERTEX_DENSITY)
        IncrementVertexDensityCounter(output.positionCS);
#endif

#if defined(VARYINGS_NEED_TEXCOORD0) || defined(VARYINGS_DS_NEED_TEXCOORD0)
    output.texCoord0 = input.uv0;
#endif
#if defined(VARYINGS_NEED_TEXCOORD1) || defined(VARYINGS_DS_NEED_TEXCOORD1)
    // output.texCoord1 = input.uv1; // 我们这里直接用uv0了
    output.texCoord1 = input.uv0;
#endif
#if defined(VARYINGS_NEED_TEXCOORD2) || defined(VARYINGS_DS_NEED_TEXCOORD2)
    output.texCoord2 = input.uv2;
#endif
#if defined(VARYINGS_NEED_TEXCOORD3) || defined(VARYINGS_DS_NEED_TEXCOORD3)
    output.texCoord3 = input.uv3;
#endif
#if defined(VARYINGS_NEED_COLOR) || defined(VARYINGS_DS_NEED_COLOR)
    output.color = input.color;
#endif

    return output;
}

VaryingsMeshType VertMesh(FrameUniforms frameUniform, RenderDataPerDraw renderData, AttributesMesh input)
{
    return VertMesh(frameUniform, renderData, input, 0.0f);
}
