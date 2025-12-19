#include "runtime/function/framework/component/mesh/mesh_renderer_component.h"

#include "runtime/resource/asset_manager/asset_manager.h"
#include "runtime/function/framework/world/world_manager.h"
#include "runtime/function/framework/material/material_manager.h"
#include "runtime/resource/res_type/components/material.h"

#include "runtime/function/framework/component/transform/transform_component.h"
#include "runtime/function/framework/object/object.h"
#include "runtime/function/global/global_context.h"

#include "runtime/core/math/moyu_math2.h"

#include "runtime/function/render/render_swap_context.h"
#include "runtime/function/render/render_system.h"
#include "runtime/function/render/render_common.h"

#include <unordered_map>

namespace MoYu
{
    MeshComponentRes _capsule_mesh = { false, "asset/objects/basic/capsule.obj", "" };
    MeshComponentRes _cone_mesh = { false, "asset/objects/basic/cone.obj", "" };
    MeshComponentRes _convexmesh_mesh = { false, "asset/objects/basic/convexmesh.obj", "" };
    MeshComponentRes _cube_mesh = { false, "asset/objects/basic/cube.obj", "" };
    MeshComponentRes _cylinder_mesh = { false, "asset/objects/basic/cylinder.obj", "" };
    MeshComponentRes _sphere_mesh = { false, "asset/objects/basic/sphere.obj", "" };
    MeshComponentRes _triangle_mesh = { false, "asset/objects/basic/triangle.obj", "" };
    MeshComponentRes _square_mesh = { false, "asset/objects/basic/square.obj", "" };

    MaterialComponentRes _pbr_mat = { "asset/objects/environment/_material/temp.material.json", false, {} };

    MeshRendererComponentRes _capsule_mesh_mat = { _capsule_mesh, _pbr_mat };
    MeshRendererComponentRes _cone_mesh_mat = { _cone_mesh, _pbr_mat };
    MeshRendererComponentRes _convexmesh_mesh_mat = { _convexmesh_mesh, _pbr_mat };
    MeshRendererComponentRes _cube_mesh_mat = { _cube_mesh, _pbr_mat };
    MeshRendererComponentRes _cylinder_mesh_mat = { _cylinder_mesh, _pbr_mat };
    MeshRendererComponentRes _sphere_mesh_mat = { _sphere_mesh, _pbr_mat };
    MeshRendererComponentRes _triangle_mesh_mat = { _triangle_mesh, _pbr_mat };
    MeshRendererComponentRes _square_mesh_mat = { _square_mesh, _pbr_mat };
    
    // 使用静态unordered_map存储映射关系，提高查找效率
    static const std::unordered_map<DefaultMeshType, std::string> mesh_type_name_map = {
        {Capsule, "Capsule"},
        {Cone, "Cone"},
        {Convexmesh, "Convexmesh"},
        {Cube, "Cube"},
        {Cylinder, "Cylinder"},
        {Sphere, "Sphere"},
        {Triangle, "Triangle"},
        {Square, "Square"}
    };

    static const std::unordered_map<DefaultMeshType, MeshRendererComponentRes> mesh_type_component_map = {
        {Capsule, _capsule_mesh_mat},
        {Cone, _cone_mesh_mat},
        {Convexmesh, _convexmesh_mesh_mat},
        {Cube, _cube_mesh_mat},
        {Cylinder, _cylinder_mesh_mat},
        {Sphere, _sphere_mesh_mat},
        {Triangle, _triangle_mesh_mat},
        {Square, _square_mesh_mat}
    };

    std::string DefaultMeshTypeToName(DefaultMeshType type)
    {
        auto it = mesh_type_name_map.find(type);
        if (it != mesh_type_name_map.end()) {
            return it->second;
        }
        return "Capsule"; // 默认返回Capsule
    }

    MeshRendererComponentRes DefaultMeshTypeToComponentRes(DefaultMeshType type)
    {
        auto it = mesh_type_component_map.find(type);
        if (it != mesh_type_component_map.end()) {
            return it->second;
        }
        return _capsule_mesh_mat; // 默认返回_capsule_mesh_mat
    }

    void MeshRendererComponent::postLoadResource(std::weak_ptr<GObject> object, const std::string json_data)
    {
        m_object = object;

        MeshRendererComponentRes mesh_renderer_res = AssetManager::loadJson<MeshRendererComponentRes>(json_data);
        updateMeshRendererRes(mesh_renderer_res);

        markInit();
    }

    void MeshRendererComponent::save(ComponentDefinitionRes& out_component_res)
    {
        // 直接设置字段值而不是创建临时变量
        out_component_res.m_type_name = "MeshRendererComponent";
        out_component_res.m_component_name = this->m_component_name;
        
        // 构造mesh renderer组件资源数据
        MeshRendererComponentRes mesh_renderer_res{};
        
        // 直接设置mesh资源字段
        mesh_renderer_res.m_mesh_res.m_is_mesh_data = m_scene_mesh.m_is_mesh_data;
        mesh_renderer_res.m_mesh_res.m_mesh_data_path = m_scene_mesh.m_mesh_data_path;
        mesh_renderer_res.m_mesh_res.m_sub_mesh_file = m_scene_mesh.m_sub_mesh_file;

        // 直接设置material资源字段
        mesh_renderer_res.m_material_res.m_material_file = m_mesh_renderer_res.m_material_res.m_material_file;
        mesh_renderer_res.m_material_res.m_is_material_init = true;

        MaterialRes mat_res_data = ToMaterialRes(m_material.m_mat_data, m_material.m_shader_name);
        mesh_renderer_res.m_material_res.m_material_serialized_json_data = AssetManager::saveJson(mat_res_data);

        out_component_res.m_component_json_data = AssetManager::saveJson(mesh_renderer_res);
    }

    void MeshRendererComponent::reset()
    {
        m_mesh_renderer_res = {};

        m_scene_mesh = {};
        m_material = {};
    }

    GameObjectComponentDesc component2SwapData(MoYu::GObjectID     game_object_id,
                                               MoYu::GComponentID  transform_component_id,
                                               TransformComponent* m_transform_component_ptr,
                                               MoYu::GComponentID  mesh_renderer_component_id,
                                               SceneMesh*          m_scene_mesh_ptr,
                                               SceneMaterial*      m_scene_mat_ptr)
    {
        glm::float4x4 transform_matrix = m_transform_component_ptr->getMatrixWorld();

        glm::float3     m_scale;
        glm::quat m_orientation;
        glm::float3     m_translation;
        glm::float3     m_skew;
        glm::float4     m_perspective;
        glm::decompose(transform_matrix, m_scale, m_orientation, m_translation, m_skew, m_perspective);

        SceneTransform scene_transform     = {};
        scene_transform.m_identifier       = SceneCommonIdentifier {game_object_id, transform_component_id};
        scene_transform.m_position         = m_translation;
        scene_transform.m_rotation         = m_orientation;
        scene_transform.m_scale            = m_scale;
        scene_transform.m_transform_matrix = transform_matrix;

        SceneMeshRenderer scene_mesh_renderer = {};
        scene_mesh_renderer.m_identifier      = SceneCommonIdentifier {game_object_id, mesh_renderer_component_id};
        scene_mesh_renderer.m_scene_mesh      = *m_scene_mesh_ptr;
        scene_mesh_renderer.m_material        = *m_scene_mat_ptr;

        GameObjectComponentDesc light_component_desc = {};
        light_component_desc.m_component_type        = ComponentType::C_Transform | ComponentType::C_MeshRenderer;
        light_component_desc.m_transform_desc        = scene_transform;
        light_component_desc.m_mesh_renderer_desc    = scene_mesh_renderer;

        return light_component_desc;
    }

    void MeshRendererComponent::tick(float delta_time)
    {
        if (m_object.expired() || this->isNone())
            return;

        std::shared_ptr<MoYu::GObject> m_obj_ptr = m_object.lock();

        RenderSwapContext& render_swap_context = g_runtime_global_context.m_render_system->getSwapContext();
        RenderSwapData& logic_swap_data = render_swap_context.getLogicSwapData();

        TransformComponent* m_transform_component_ptr = m_obj_ptr->getTransformComponent().get();

        MoYu::GObjectID game_object_id = m_obj_ptr->getID();
        MoYu::GComponentID transform_component_id = m_transform_component_ptr->getComponentId();
        MoYu::GComponentID mesh_renderer_component_id = this->getComponentId();

        if (this->isToErase())
        {
            // 直接构造对象而不是调用函数，减少临时对象创建
            glm::float4x4 transform_matrix = m_transform_component_ptr->getMatrixWorld();

            glm::float3 m_scale;
            glm::quat m_orientation;
            glm::float3 m_translation;
            glm::float3 m_skew;
            glm::float4 m_perspective;
            glm::decompose(transform_matrix, m_scale, m_orientation, m_translation, m_skew, m_perspective);

            SceneTransform scene_transform = {};
            scene_transform.m_identifier = SceneCommonIdentifier{ game_object_id, transform_component_id };
            scene_transform.m_position = m_translation;
            scene_transform.m_rotation = m_orientation;
            scene_transform.m_scale = m_scale;
            scene_transform.m_transform_matrix = transform_matrix;

            SceneMeshRenderer scene_mesh_renderer = {};
            scene_mesh_renderer.m_identifier = SceneCommonIdentifier{ game_object_id, mesh_renderer_component_id };
            scene_mesh_renderer.m_scene_mesh = m_scene_mesh;
            scene_mesh_renderer.m_material = m_material;

            GameObjectComponentDesc mesh_renderer_desc = {};
            mesh_renderer_desc.m_component_type = ComponentType::C_Transform | ComponentType::C_MeshRenderer;
            mesh_renderer_desc.m_transform_desc = scene_transform;
            mesh_renderer_desc.m_mesh_renderer_desc = scene_mesh_renderer;

            logic_swap_data.addDeleteGameObject({ game_object_id, {mesh_renderer_desc} });

            this->markNone();
        }
        else if (m_transform_component_ptr->isMatrixDirty() || this->isDirty())
        {
            // 直接构造对象而不是调用函数，减少临时对象创建
            glm::float4x4 transform_matrix = m_transform_component_ptr->getMatrixWorld();

            glm::float3 m_scale;
            glm::quat m_orientation;
            glm::float3 m_translation;
            glm::float3 m_skew;
            glm::float4 m_perspective;
            glm::decompose(transform_matrix, m_scale, m_orientation, m_translation, m_skew, m_perspective);

            SceneTransform scene_transform = {};
            scene_transform.m_identifier = SceneCommonIdentifier{ game_object_id, transform_component_id };
            scene_transform.m_position = m_translation;
            scene_transform.m_rotation = m_orientation;
            scene_transform.m_scale = m_scale;
            scene_transform.m_transform_matrix = transform_matrix;

            SceneMeshRenderer scene_mesh_renderer = {};
            scene_mesh_renderer.m_identifier = SceneCommonIdentifier{ game_object_id, mesh_renderer_component_id };
            scene_mesh_renderer.m_scene_mesh = m_scene_mesh;
            scene_mesh_renderer.m_material = m_material;

            GameObjectComponentDesc mesh_renderer_desc = {};
            mesh_renderer_desc.m_component_type = ComponentType::C_Transform | ComponentType::C_MeshRenderer;
            mesh_renderer_desc.m_transform_desc = scene_transform;
            mesh_renderer_desc.m_mesh_renderer_desc = scene_mesh_renderer;

            logic_swap_data.addDirtyGameObject({ game_object_id, {mesh_renderer_desc} });

            this->markIdle();
        }
    }

    void MeshRendererComponent::updateMeshRendererRes(const MeshRendererComponentRes& res)
    {
        bool is_dirty = false;
        
        // 检查网格资源是否有变化
        if (m_mesh_renderer_res.m_mesh_res.m_is_mesh_data != res.m_mesh_res.m_is_mesh_data ||
            m_mesh_renderer_res.m_mesh_res.m_mesh_data_path != res.m_mesh_res.m_mesh_data_path ||
            m_mesh_renderer_res.m_mesh_res.m_sub_mesh_file != res.m_mesh_res.m_sub_mesh_file)
        {
            m_mesh_renderer_res.m_mesh_res = res.m_mesh_res;
            m_scene_mesh = { m_mesh_renderer_res.m_mesh_res.m_is_mesh_data,
                            m_mesh_renderer_res.m_mesh_res.m_sub_mesh_file,
                            m_mesh_renderer_res.m_mesh_res.m_mesh_data_path };
            is_dirty = true;
        }

        // 检查材质资源是否有变化
        if (m_mesh_renderer_res.m_material_res.m_material_file != res.m_material_res.m_material_file ||
            m_mesh_renderer_res.m_material_res.m_is_material_init != res.m_material_res.m_is_material_init ||
            m_mesh_renderer_res.m_material_res.m_material_serialized_json_data != res.m_material_res.m_material_serialized_json_data)
        {
            m_mesh_renderer_res.m_material_res = res.m_material_res;

            MaterialManager* m_mat_manager_ptr = g_runtime_global_context.m_material_manager.get();

            MaterialRes m_mat_res = m_mat_manager_ptr->loadMaterialRes(m_mesh_renderer_res.m_material_res.m_material_file);

            if (m_mesh_renderer_res.m_material_res.m_is_material_init)
            {
                std::string m_material_serialized_json = m_mesh_renderer_res.m_material_res.m_material_serialized_json_data;
                MaterialRes mat_res_data = AssetManager::loadJson<MaterialRes>(m_material_serialized_json);
                m_mat_res = mat_res_data;
            }

            StandardLightMaterial m_mat_data = ToStandardMaterial(m_mat_res);

            m_material.m_shader_name = m_mat_res._ShaderName;
            m_material.m_mat_data = m_mat_data;
            
            is_dirty = true;
        }

        if (is_dirty)
        {
            markDirty();
        }
    }

    void MeshRendererComponent::updateMeshRes(std::string mesh_file_path)
    {
        // 只有当mesh文件路径确实发生变化时才更新
        if (m_mesh_renderer_res.m_mesh_res.m_mesh_data_path == mesh_file_path)
            return;

        MeshComponentRes m_mesh_res = { false, mesh_file_path, "" };
        m_mesh_renderer_res.m_mesh_res = m_mesh_res;

        m_scene_mesh.m_is_mesh_data = m_mesh_renderer_res.m_mesh_res.m_is_mesh_data;
        m_scene_mesh.m_sub_mesh_file = m_mesh_renderer_res.m_mesh_res.m_sub_mesh_file;
        m_scene_mesh.m_mesh_data_path = m_mesh_renderer_res.m_mesh_res.m_mesh_data_path;

        markDirty();
    }

    void MeshRendererComponent::updateMaterial(std::string material_path, std::string serialized_json_str)
    {
        // 只有当材质路径确实发生变化时才更新
        if (m_mesh_renderer_res.m_material_res.m_material_file == material_path &&
            m_mesh_renderer_res.m_material_res.m_material_serialized_json_data == serialized_json_str)
            return;

        MaterialComponentRes m_material_res = {
            material_path, serialized_json_str.empty() ? true : false, serialized_json_str };
        m_mesh_renderer_res.m_material_res = m_material_res;

        MaterialManager* m_mat_manager_ptr = g_runtime_global_context.m_material_manager.get();
        MaterialRes m_mat_res = m_mat_manager_ptr->loadMaterialRes(m_mesh_renderer_res.m_material_res.m_material_file);

        StandardLightMaterial m_mat_data = ToStandardMaterial(m_mat_res);

        m_material.m_shader_name = m_mat_res._ShaderName;
        m_material.m_mat_data = m_mat_data;

        markDirty();
    }

} // namespace MoYu
