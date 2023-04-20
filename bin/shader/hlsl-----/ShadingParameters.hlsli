#pragma once

//------------------------------------------------------------------------------
// Material evaluation
//------------------------------------------------------------------------------

/**
 * Computes global shading parameters used to apply lighting, such as the view
 * vector in world space, the tangent frame at the shading point, etc.
 */
void computeShadingParams()
{
#if defined(HAS_ATTRIBUTE_TANGENTS)
    float3 n = vertex_worldNormal;
#if defined(MATERIAL_NEEDS_TBN)
    float3 t = vertex_worldTangent.xyz;
    float3 b = cross(n, t) * sign(vertex_worldTangent.w);
#endif

#if defined(MATERIAL_HAS_DOUBLE_SIDED_CAPABILITY)
    if (isDoubleSided())
    {
        n = SV_IsFrontFace ? n : -n;
#if defined(MATERIAL_NEEDS_TBN)
        t = SV_IsFrontFace ? t : -t;
        b = SV_IsFrontFace ? b : -b;
#endif
    }
#endif

    shading_geometricNormal = normalize(n);

#if defined(MATERIAL_NEEDS_TBN)
    // We use unnormalized post-interpolation values, assuming mikktspace tangents
    shading_tangentToWorld = mat3(t, b, n);
#endif
#endif

    shading_position = vertex_worldPosition.xyz;

    // With perspective camera, the view vector is cast from the fragment pos to the eye position,
    // With ortho camera, however, the view vector is the same for all fragments:
    float3 sv = isPerspectiveProjection() ? (frameUniforms.worldFromViewMatrix[3].xyz - shading_position) :
                                                frameUniforms.worldFromViewMatrix[2].xyz; // ortho camera backward dir
    shading_view  = normalize(sv);

    // we do this so we avoid doing (matrix multiply), but we burn 4 varyings:
    //    p = clipFromWorldMatrix * shading_position;
    //    shading_normalizedViewportCoord = p.xy * 0.5 / p.w + 0.5
    shading_normalizedViewportCoord = vertex_position.xy * (0.5 / vertex_position.w) + 0.5;
}

/**
 * Computes global shading parameters that the material might need to access
 * before lighting: N dot V, the reflected vector and the shading normal (before
 * applying the normal map). These parameters can be useful to material authors
 * to compute other material properties.
 *
 * This function must be invoked by the user's material code (guaranteed by
 * the material compiler) after setting a value for MaterialInputs.normal.
 */
void prepareMaterial(const MaterialInputs material)
{
#if defined(HAS_ATTRIBUTE_TANGENTS)
#if defined(MATERIAL_HAS_NORMAL)
    shading_normal = normalize(shading_tangentToWorld * material.normal);
#else
    shading_normal = getWorldGeometricNormalVector();
#endif
    shading_NoV       = clampNoV(dot(shading_normal, shading_view));
    shading_reflected = reflect(-shading_view, shading_normal);

#if defined(MATERIAL_HAS_BENT_NORMAL)
    shading_bentNormal = normalize(shading_tangentToWorld * material.bentNormal);
#endif

#if defined(MATERIAL_HAS_CLEAR_COAT)
#if defined(MATERIAL_HAS_CLEAR_COAT_NORMAL)
    shading_clearCoatNormal = normalize(shading_tangentToWorld * material.clearCoatNormal);
#else
    shading_clearCoatNormal = getWorldGeometricNormalVector();
#endif
#endif
#endif
}


