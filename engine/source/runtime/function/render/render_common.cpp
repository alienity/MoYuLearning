#include "render_common.h"
#include "runtime/resource/res_type/components/material.h"

namespace MoYu
{
    SceneCommonIdentifier _UndefCommonIdentifier {K_Invalid_Object_Id, K_Invalid_Component_Id};

    ScenePBRMaterial _DefaultScenePBRMaterial = {
        false,
        false,
        {1.0f, 1.0f, 1.0f, 1.0f},
        {1.0f},
        {1.0f},
        {1.0f},
        {1.0f},
        {0.0f, 0.0f, 0.0f},
        {true, true, 0, "asset/objects/_textures/white.tga"},
        {false, true, 0, "asset/objects/_textures/mr.tga"},
        {false, true, 0, "asset/objects/_textures/n.tga"},
        {false, true, 0, ""},
        {false, true, 0, ""},
    };

    MaterialRes ToMaterialRes(const ScenePBRMaterial& pbrMaterial, const std::string shaderName)
    {
        MaterialRes m_mat_data = {shaderName,
                                  pbrMaterial.m_blend,
                                  pbrMaterial.m_double_sided,
                                  pbrMaterial.m_base_color_factor,
                                  pbrMaterial.m_metallic_factor,
                                  pbrMaterial.m_roughness_factor,
                                  pbrMaterial.m_normal_scale,
                                  pbrMaterial.m_occlusion_strength,
                                  pbrMaterial.m_emissive_factor,
                                  pbrMaterial.m_base_color_texture_file,
                                  pbrMaterial.m_metallic_roughness_texture_file,
                                  pbrMaterial.m_normal_texture_file,
                                  pbrMaterial.m_occlusion_texture_file,
                                  pbrMaterial.m_emissive_texture_file};
        return m_mat_data;
    }

    ScenePBRMaterial ToPBRMaterial(const MaterialRes& materialRes)
    {
        ScenePBRMaterial m_mat_data = {materialRes.m_blend,
                                       materialRes.m_double_sided,
                                       materialRes.m_base_color_factor,
                                       materialRes.m_metallic_factor,
                                       materialRes.m_roughness_factor,
                                       materialRes.m_normal_scale,
                                       materialRes.m_occlusion_strength,
                                       materialRes.m_emissive_factor,
                                       materialRes.m_base_color_texture_file,
                                       materialRes.m_metallic_roughness_texture_file,
                                       materialRes.m_normal_texture_file,
                                       materialRes.m_occlusion_texture_file,
                                       materialRes.m_emissive_texture_file};
        return m_mat_data;
    }

    // clang-format off

    //--------------------------------------------------------------------------------------
    // Vertex struct holding position information.
    const D3D12_INPUT_ELEMENT_DESC D3D12MeshVertexPosition::InputElements[] =
    {
        { "POSITION",    0, DXGI_FORMAT_R32G32B32_FLOAT,    0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
    };

    static_assert(sizeof(D3D12MeshVertexPosition) == 12, "Vertex struct/layout mismatch");

    const RHI::D3D12InputLayout D3D12MeshVertexPosition::InputLayout = 
        RHI::D3D12InputLayout( D3D12MeshVertexPosition::InputElements, 
            D3D12MeshVertexPosition::InputElementCount);

    //--------------------------------------------------------------------------------------
    // Vertex struct holding position and texture mapping information.
    const D3D12_INPUT_ELEMENT_DESC D3D12MeshVertexPositionTexture::InputElements[] =
    {
        { "POSITION",    0, DXGI_FORMAT_R32G32B32_FLOAT,    0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "TEXCOORD",    0, DXGI_FORMAT_R32G32_FLOAT,       0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
    };

    static_assert(sizeof(D3D12MeshVertexPositionTexture) == 20, "Vertex struct/layout mismatch");

    const RHI::D3D12InputLayout D3D12MeshVertexPositionTexture::InputLayout = 
        RHI::D3D12InputLayout( D3D12MeshVertexPositionTexture::InputElements, 
            D3D12MeshVertexPositionTexture::InputElementCount);

    //--------------------------------------------------------------------------------------
    // Vertex struct holding position, normal, tangent and texture mapping information.
    const D3D12_INPUT_ELEMENT_DESC D3D12MeshVertexPositionNormalTangentTexture::InputElements[] =
    {
        { "POSITION",    0, DXGI_FORMAT_R32G32B32_FLOAT,    0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "NORMAL",      0, DXGI_FORMAT_R32G32B32_FLOAT,    0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "TANGENT",     0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "TEXCOORD",    0, DXGI_FORMAT_R32G32_FLOAT,       0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
    };

    static_assert(sizeof(D3D12MeshVertexPositionNormalTangentTexture) == 48, "Vertex struct/layout mismatch");

    const RHI::D3D12InputLayout D3D12MeshVertexPositionNormalTangentTexture::InputLayout = 
        RHI::D3D12InputLayout( D3D12MeshVertexPositionNormalTangentTexture::InputElements, 
            D3D12MeshVertexPositionNormalTangentTexture::InputElementCount);

    //--------------------------------------------------------------------------------------
    // Vertex struct holding position, normal, tangent and texture mapping information.
    const D3D12_INPUT_ELEMENT_DESC D3D12MeshVertexPositionNormalTangentTextureJointBinding::InputElements[] =
    {
        { "POSITION",    0, DXGI_FORMAT_R32G32B32_FLOAT,    0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "NORMAL",      0, DXGI_FORMAT_R32G32B32_FLOAT,    0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "TANGENT",     0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "TEXCOORD",    0, DXGI_FORMAT_R32G32_FLOAT,       0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "BLENDINDICES",0, DXGI_FORMAT_R32G32B32A32_UINT,  0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "BLENDWEIGHT", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
    };

    static_assert(sizeof(D3D12MeshVertexPositionNormalTangentTextureJointBinding) == 80, "Vertex struct/layout mismatch");

    const RHI::D3D12InputLayout D3D12MeshVertexPositionNormalTangentTextureJointBinding::InputLayout = 
        RHI::D3D12InputLayout( D3D12MeshVertexPositionNormalTangentTextureJointBinding::InputElements, 
            D3D12MeshVertexPositionNormalTangentTextureJointBinding::InputElementCount);

    // clang-format on



}