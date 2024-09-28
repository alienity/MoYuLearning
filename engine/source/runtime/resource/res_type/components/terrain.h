#pragma once
#include "runtime/resource/res_type/common_serializer.h"
#include "runtime/resource/res_type/components/material.h"
#include "runtime/resource/res_type/components/mesh_renderer.h"
#include <vector>

namespace MoYu
{

    struct TerrainComponentRes
    {
        glm::int3 terrain_size {glm::int3(10240, 2048, 10240)}; // 10240*2048*10240

        //int max_node_id = 34124; //5x5+10x10+20x20+40x40+80x80+160x160 - 1
        int max_terrain_lod = 5; // ����lod�ȼ�
        int max_lod_node_count = 5; // ÿ��lod��ӵ�е�node��

        int patch_mesh_grid_count = 16; // PatchMesh��16x16�������
        int patch_mesh_size = 8; // PatchMesh�߳�8��
        int patch_count_per_node = 8; // Node���8x8��Patch

        //float patch_mesh_grid_size = 0.5f; // patch_mesh_size / patch_mesh_grid_count
        //int sector_count_terrain = 160; // terrain_size.x / (patch_mesh_size * patch_count_per_node);

        SceneImage m_heightmap_file {};
        SceneImage m_normalmap_file {};

        MaterialComponentRes m_material_res{};
    };
    NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(TerrainComponentRes,
                                                    terrain_size,
                                                    max_terrain_lod,
                                                    max_lod_node_count,
                                                    patch_mesh_grid_count,
                                                    patch_mesh_size,
                                                    patch_count_per_node,
                                                    m_heightmap_file,
                                                    m_normalmap_file,
                                                    m_material_res)

} // namespace MoYu