#pragma once
#include "runtime/resource/res_type/common_serializer.h"
#include <vector>

namespace MoYu
{
    struct MeshComponentRes
    {
        bool m_is_mesh_data {false};
        std::string m_sub_mesh_file {""};
        std::string m_mesh_data_path {""};
    };
    NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(MeshComponentRes, m_is_mesh_data, m_sub_mesh_file, m_mesh_data_path)

    struct MaterialComponentRes
    {
        std::string m_material_file {""};
        bool m_is_material_init {false};
        std::vector<uint64_t> m_material_serialized_data;
    };
    NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(MaterialComponentRes,
                                       m_material_file,
                                       m_is_material_init,
                                       m_material_serialized_data)

    struct MeshRendererComponentRes
    {
        MeshComponentRes     m_mesh_res {};
        MaterialComponentRes m_material_res {};
    };
    NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(MeshRendererComponentRes, m_mesh_res, m_material_res)
} // namespace MoYu