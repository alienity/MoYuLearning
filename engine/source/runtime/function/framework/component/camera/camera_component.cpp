#include "runtime/function/framework/component/camera/camera_component.h"

#include "runtime/core/base/macro.h"
#include "runtime/function/global/global_context.h"
#include "runtime/core/math/moyu_math2.h"

#include "runtime/resource/asset_manager/asset_manager.h"

#include "runtime/function/framework/component/transform/transform_component.h"
#include "runtime/function/framework/level/level.h"
#include "runtime/function/framework/object/object.h"
#include "runtime/function/framework/world/world_manager.h"
#include "runtime/function/global/global_context.h"
#include "runtime/function/input/input_system.h"

#include "runtime/function/render/render_system.h"

#include "runtime/resource/res_type/components/camera.h"

namespace MoYu
{
    void CameraComponent::reset()
    {
        m_camera_res = {};
    }

    void CameraComponent::postLoadResource(std::weak_ptr<GObject> object, const std::string json_data)
    {
        m_object = object;

        CameraComponentRes camera_res = AssetManager::loadJson<CameraComponentRes>(json_data);
        m_camera_res = camera_res;

        const std::string& camera_type_name = m_camera_res.m_CamParamName;
        if (camera_type_name == "FirstPersonCameraParameter")
        {
            m_camera_mode = CameraMode::first_person;
        }
        else if (camera_type_name == "ThirdPersonCameraParameter")
        {
            m_camera_mode = CameraMode::third_person;
        }
        else if (camera_type_name == "FreeCameraParameter")
        {
            m_camera_mode = CameraMode::free;
        }
        else
        {
            LOG_ERROR("invalid camera type");
        }

        RenderSwapContext& swap_context = g_runtime_global_context.m_render_system->getSwapContext();

        CameraSwapData camera_swap_data;
        camera_swap_data.m_width  = m_camera_res.m_FreeCamParam.m_width;
        camera_swap_data.m_height = m_camera_res.m_FreeCamParam.m_height;
        camera_swap_data.m_nearZ  = m_camera_res.m_FreeCamParam.m_nearZ;
        camera_swap_data.m_farZ   = m_camera_res.m_FreeCamParam.m_farZ;
        camera_swap_data.m_fov_y  = m_camera_res.m_FreeCamParam.m_fovY;
        camera_swap_data.m_is_perspective = m_camera_res.m_FreeCamParam.m_perspective;

        swap_context.getLogicSwapData().m_camera_swap_data = camera_swap_data;
    }

    void CameraComponent::save(ComponentDefinitionRes& out_component_res)
    {
        CameraComponentRes camera_res {};
        (&camera_res)->m_CamParamName        = m_camera_res.m_CamParamName;
        (&camera_res)->m_FirstPersonCamParam = m_camera_res.m_FirstPersonCamParam;
        (&camera_res)->m_ThirdPersonCamParam = m_camera_res.m_ThirdPersonCamParam;
        (&camera_res)->m_FreeCamParam        = m_camera_res.m_FreeCamParam;

        out_component_res.m_type_name           = "CameraComponent";
        out_component_res.m_component_name      = this->m_component_name;
        out_component_res.m_component_json_data = AssetManager::saveJson(camera_res);
    }

    void CameraComponent::tick(float delta_time)
    {
        if (m_object.expired())
            return;

        //std::shared_ptr<Level> current_level = g_runtime_global_context.m_world_manager->getCurrentActiveLevel().lock();
        //std::shared_ptr<Character> current_character = current_level->getCurrentActiveCharacter().lock();
        //if (current_character == nullptr)
        //    return;

        //if (current_character->getObjectID() != m_parent_object.lock()->getID())
        //    return;

        switch (m_camera_mode)
        {
            case CameraMode::first_person:
                tickFirstPersonCamera(delta_time);
                break;
            case CameraMode::third_person:
                tickThirdPersonCamera(delta_time);
                break;
            case CameraMode::free:
                break;
            default:
                break;
        }
    }

    void CameraComponent::tickFirstPersonCamera(float delta_time)
    {
        /*
        std::shared_ptr<Level> current_level = g_runtime_global_context.m_world_manager->getCurrentActiveLevel().lock();
        std::shared_ptr<Character> current_character = current_level->getCurrentActiveCharacter().lock();
        if (current_character == nullptr)
            return;

        Quaternion q_yaw = Quaternion::fromAxisAngle(
            Vector3::UnitY, g_runtime_global_context.m_input_system->m_cursor_delta_yaw.m_rad);
        Quaternion q_pitch = Quaternion::fromAxisAngle(
            Vector3::UnitX, g_runtime_global_context.m_input_system->m_cursor_delta_pitch.m_rad);

        const float offset  = static_cast<FirstPersonCameraParameter*>(m_camera_res.m_parameter)->m_vertical_offset;
        Vector3     eye_pos = current_character->getPosition() + offset * Vector3::UnitY;

        m_foward = q_yaw * q_pitch * m_foward;
        m_left   = q_yaw * q_pitch * m_left;
        m_up     = m_foward.cross(m_left);

        Matrix4x4 desired_mat = Math::makeLookAtMatrix(eye_pos, m_foward, m_up);

        RenderSwapContext& swap_context = g_runtime_global_context.m_render_system->getSwapContext();
        CameraSwapData     camera_swap_data;
        camera_swap_data.m_camera_type                     = RenderCameraType::Motor;
        camera_swap_data.m_view_matrix                     = desired_mat; // ��Ҫת��
        swap_context.getLogicSwapData().m_camera_swap_data = camera_swap_data;

        Vector3    object_facing = m_foward - m_foward.dot(Vector3::Up) * Vector3::Up;
        Vector3    object_left   = Vector3::Up.cross(object_facing);
        Quaternion object_rotation = Quaternion::fromAxes(-object_left, Vector3::Up, -object_facing);
        current_character->setRotation(object_rotation);
        */
    }

    void CameraComponent::tickThirdPersonCamera(float delta_time)
    {
        /*
        std::shared_ptr<Level> current_level = g_runtime_global_context.m_world_manager->getCurrentActiveLevel().lock();
        std::shared_ptr<Character> current_character = current_level->getCurrentActiveCharacter().lock();
        if (current_character == nullptr)
            return;

        ThirdPersonCameraParameter* param = static_cast<ThirdPersonCameraParameter*>(m_camera_res.m_parameter);

        Quaternion q_yaw = Quaternion::fromAxisAngle(
            Vector3::UnitY, g_runtime_global_context.m_input_system->m_cursor_delta_yaw.m_rad);
        Quaternion q_pitch = Quaternion::fromAxisAngle(
            Vector3::UnitX, g_runtime_global_context.m_input_system->m_cursor_delta_pitch.m_rad);

        param->m_cursor_pitch = q_pitch * param->m_cursor_pitch;

        const float vertical_offset   = param->m_vertical_offset;
        const float horizontal_offset = param->m_horizontal_offset;
        Vector3     offset            = Vector3(0, vertical_offset, horizontal_offset);

        Vector3 center_pos = current_character->getPosition() + Vector3::Up * vertical_offset;
        Vector3 camera_pos =
            current_character->getRotation() * param->m_cursor_pitch * offset + current_character->getPosition();
        Vector3 camera_forward = center_pos - camera_pos;
        Vector3 camera_up      = current_character->getRotation() * param->m_cursor_pitch * Vector3::Up;

        current_character->setRotation(q_yaw * current_character->getRotation());

        Matrix4x4 desired_mat = Math::makeLookAtMatrix(camera_pos, camera_pos + camera_forward, camera_up);

        RenderSwapContext& swap_context = g_runtime_global_context.m_render_system->getSwapContext();
        CameraSwapData     camera_swap_data;
        camera_swap_data.m_camera_type                     = RenderCameraType::Motor;
        camera_swap_data.m_view_matrix                     = desired_mat;
        swap_context.getLogicSwapData().m_camera_swap_data = camera_swap_data;
        */
    }

    void CameraComponent::tickFreeCamera(float delta_time)
    {

    }

} // namespace MoYu
