#include "runtime/function/framework/component/camera/camera_component.h"

#include "runtime/core/base/macro.h"
#include "runtime/function/global/global_context.h"
#include "runtime/core/math/moyu_math2.h"

#include "runtime/resource/asset_manager/asset_manager.h"

#include "runtime/function/framework/component/transform/transform_component.h"
#include "runtime/function/framework/level/level.h"
#include "runtime/function/framework/object/object.h"
#include "runtime/function/framework/world/world_manager.h"
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

        // Validate parameters
        if (m_camera_res.m_CamParamName.empty()) {
            m_camera_res.m_CamParamName = FreeCameraParameterName; // Default to free camera
        }

        // Initialize camera based on the camera type
        const std::string& camera_type_name = m_camera_res.m_CamParamName;
        
        // Set initial camera parameters based on the selected camera type
        if (camera_type_name == FreeCameraParameterName) {
            const FreeCameraParameter& free_cam_param = m_camera_res.m_FreeCamParam;
            
            // Validate parameters
            float fovY = free_cam_param.m_fovY;
            if (fovY <= 0.0f || fovY >= 180.0f) fovY = 45.0f;
            
            float aspectRatio = 16.0f/9.0f;
            if (free_cam_param.m_width > 0 && free_cam_param.m_height > 0) {
                aspectRatio = static_cast<float>(free_cam_param.m_width) / static_cast<float>(free_cam_param.m_height);
            }
            
            float nearZ = free_cam_param.m_nearZ;
            if (nearZ <= 0.0f) nearZ = 0.1f;
            
            float farZ = free_cam_param.m_farZ;
            if (farZ <= nearZ) farZ = 100.0f;
            
            float speed = free_cam_param.m_speed;
            if (speed <= 0.0f) speed = 2.5f;
            
            m_camera.setPerspective(fovY, aspectRatio, nearZ, farZ);
            m_camera.setMovementSpeed(speed);
        } else if (camera_type_name == FirstPersonCameraParameterName) {
            const FirstPersonCameraParameter& fpv_cam_param = m_camera_res.m_FirstPersonCamParam;
            
            // Validate parameters
            float fovY = fpv_cam_param.m_fovY;
            if (fovY <= 0.0f || fovY >= 180.0f) fovY = 45.0f;
            
            float aspectRatio = 16.0f/9.0f;
            if (fpv_cam_param.m_width > 0 && fpv_cam_param.m_height > 0) {
                aspectRatio = static_cast<float>(fpv_cam_param.m_width) / static_cast<float>(fpv_cam_param.m_height);
            }
            
            float nearZ = fpv_cam_param.m_nearZ;
            if (nearZ <= 0.0f) nearZ = 0.1f;
            
            float farZ = fpv_cam_param.m_farZ;
            if (farZ <= nearZ) farZ = 100.0f;
            
            m_camera.setPerspective(fovY, aspectRatio, nearZ, farZ);
        } else if (camera_type_name == ThirdPersonCameraParameterName) {
            const ThirdPersonCameraParameter& tpv_cam_param = m_camera_res.m_ThirdPersonCamParam;
            
            // Validate parameters
            float fovY = tpv_cam_param.m_fovY;
            if (fovY <= 0.0f || fovY >= 180.0f) fovY = 45.0f;
            
            float aspectRatio = 16.0f/9.0f;
            if (tpv_cam_param.m_width > 0 && tpv_cam_param.m_height > 0) {
                aspectRatio = static_cast<float>(tpv_cam_param.m_width) / static_cast<float>(tpv_cam_param.m_height);
            }
            
            float nearZ = tpv_cam_param.m_nearZ;
            if (nearZ <= 0.0f) nearZ = 0.1f;
            
            float farZ = tpv_cam_param.m_farZ;
            if (farZ <= nearZ) farZ = 100.0f;
            
            m_camera.setPerspective(fovY, aspectRatio, nearZ, farZ);
        }

        // Send initial camera data to render system
        updateCameraSwapData();
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

    void CameraComponent::updateCameraSwapData()
    {
        RenderSwapContext& swap_context = g_runtime_global_context.m_render_system->getSwapContext();
        
        CameraSwapData camera_swap_data;
        camera_swap_data.m_camera_type = RenderCameraType::Motor;
        camera_swap_data.m_view_matrix = m_camera.getViewMatrix();
        
        // Set parameters based on camera type
        const std::string& camera_type_name = m_camera_res.m_CamParamName;
        if (camera_type_name == FreeCameraParameterName) {
            camera_swap_data.m_width  = m_camera_res.m_FreeCamParam.m_width;
            camera_swap_data.m_height = m_camera_res.m_FreeCamParam.m_height;
            camera_swap_data.m_nearZ  = m_camera_res.m_FreeCamParam.m_nearZ;
            camera_swap_data.m_farZ   = m_camera_res.m_FreeCamParam.m_farZ;
            camera_swap_data.m_fov_y  = m_camera_res.m_FreeCamParam.m_fovY;
            camera_swap_data.m_is_perspective = m_camera_res.m_FreeCamParam.m_perspective;
        } else if (camera_type_name == FirstPersonCameraParameterName) {
            camera_swap_data.m_width  = m_camera_res.m_FirstPersonCamParam.m_width;
            camera_swap_data.m_height = m_camera_res.m_FirstPersonCamParam.m_height;
            camera_swap_data.m_nearZ  = m_camera_res.m_FirstPersonCamParam.m_nearZ;
            camera_swap_data.m_farZ   = m_camera_res.m_FirstPersonCamParam.m_farZ;
            camera_swap_data.m_fov_y  = m_camera_res.m_FirstPersonCamParam.m_fovY;
            camera_swap_data.m_is_perspective = true;
        } else if (camera_type_name == ThirdPersonCameraParameterName) {
            camera_swap_data.m_width  = m_camera_res.m_ThirdPersonCamParam.m_width;
            camera_swap_data.m_height = m_camera_res.m_ThirdPersonCamParam.m_height;
            camera_swap_data.m_nearZ  = m_camera_res.m_ThirdPersonCamParam.m_nearZ;
            camera_swap_data.m_farZ   = m_camera_res.m_ThirdPersonCamParam.m_farZ;
            camera_swap_data.m_fov_y  = m_camera_res.m_ThirdPersonCamParam.m_fovY;
            camera_swap_data.m_is_perspective = true;
        }
        
        swap_context.getLogicSwapData().m_camera_swap_data = camera_swap_data;
    }

    void CameraComponent::tick(float delta_time)
    {
        if (m_object.expired())
            return;

        // Based on camera type, call appropriate tick function
        const std::string& camera_type_name = m_camera_res.m_CamParamName;
        
        if (camera_type_name == FreeCameraParameterName) {
            tickFreeCamera(delta_time);
        } else if (camera_type_name == FirstPersonCameraParameterName) {
            tickFirstPersonCamera(delta_time);
        } else if (camera_type_name == ThirdPersonCameraParameterName) {
            tickThirdPersonCamera(delta_time);
        }
    }

    void CameraComponent::tickFirstPersonCamera(float delta_time)
    {
        // Check if object is still valid
        if (m_object.expired())
            return;
            
        // Get input system
        auto input_system = g_runtime_global_context.m_input_system;
        if (!input_system)
            return;
        
        // Get transform component of the camera object
        std::shared_ptr<GObject> object = m_object.lock();
        if (!object)
            return;
            
        auto transform_component = object->getTransformComponent();
        if (!transform_component)
            return;
            
        // Handle keyboard movement for changing vertical offset
        if (input_system->isKeyPressing(KeyBoardButton::R)) {
            m_camera_res.m_FirstPersonCamParam.m_vertical_offset += 0.1f;
        }
        if (input_system->isKeyPressing(KeyBoardButton::F)) {
            m_camera_res.m_FirstPersonCamParam.m_vertical_offset -= 0.1f;
        }
            
        // Update camera rotation based on mouse input
        float yaw_offset = input_system->m_cursor_delta_yaw * 100.0f;
        float pitch_offset = input_system->m_cursor_delta_pitch * 100.0f;
        
        // Apply rotation to the camera
        m_camera.processMouseMovement(yaw_offset, pitch_offset);
        
        // Update camera position to match the object position
        glm::float3 object_position = transform_component->getPosition();
        const float vertical_offset = m_camera_res.m_FirstPersonCamParam.m_vertical_offset;
        glm::float3 eye_position = object_position + glm::float3(0.0f, vertical_offset, 0.0f);
        m_camera.setPosition(eye_position);
        
        // Send camera data to render system
        updateCameraSwapData();
    }

    void CameraComponent::tickThirdPersonCamera(float delta_time)
    {
        // Check if object is still valid
        if (m_object.expired())
            return;
            
        // Get input system
        auto input_system = g_runtime_global_context.m_input_system;
        if (!input_system)
            return;
        
        // Get transform component of the camera object
        std::shared_ptr<GObject> object = m_object.lock();
        if (!object)
            return;
            
        auto transform_component = object->getTransformComponent();
        if (!transform_component)
            return;
            
        // Handle keyboard input for adjusting camera distance and height
        if (input_system->isKeyPressing(KeyBoardButton::KEY_0)) { // 0 key to increase distance
            m_camera_res.m_ThirdPersonCamParam.m_horizontal_offset += 0.5f;
        }
        if (input_system->isKeyPressing(KeyBoardButton::KEY_9)) { // 9 key to decrease distance
            m_camera_res.m_ThirdPersonCamParam.m_horizontal_offset -= 0.5f;
            if (m_camera_res.m_ThirdPersonCamParam.m_horizontal_offset < 1.0f)
                m_camera_res.m_ThirdPersonCamParam.m_horizontal_offset = 1.0f;
        }
        if (input_system->isKeyPressing(KeyBoardButton::R)) {
            m_camera_res.m_ThirdPersonCamParam.m_vertical_offset += 0.1f;
        }
        if (input_system->isKeyPressing(KeyBoardButton::F)) {
            m_camera_res.m_ThirdPersonCamParam.m_vertical_offset -= 0.1f;
        }
            
        // Update camera rotation based on mouse input
        float yaw_offset = input_system->m_cursor_delta_yaw * 100.0f;
        float pitch_offset = input_system->m_cursor_delta_pitch * 100.0f;
        
        // Apply rotation to the camera
        m_camera.processMouseMovement(yaw_offset, pitch_offset);
        
        // Calculate camera position based on object position and offset
        glm::float3 object_position = transform_component->getPosition();
        const float vertical_offset = m_camera_res.m_ThirdPersonCamParam.m_vertical_offset;
        const float horizontal_offset = m_camera_res.m_ThirdPersonCamParam.m_horizontal_offset;
        
        // Get camera forward direction (opposite to where it's looking)
        glm::float3 camera_reverse_direction = -m_camera.getFront(); 
        
        // Position camera behind the object
        glm::float3 camera_position = object_position + 
                                     glm::float3(0.0f, vertical_offset, 0.0f) + 
                                     camera_reverse_direction * horizontal_offset;
                                      
        m_camera.setPosition(camera_position);
        
        // Make camera look at the object
        m_camera.lookAt(object_position + glm::float3(0.0f, vertical_offset, 0.0f));
        
        // Send camera data to render system
        updateCameraSwapData();
    }

    void CameraComponent::tickFreeCamera(float delta_time)
    {
        // Check if object is still valid
        if (m_object.expired())
            return;
            
        // Get input system
        auto input_system = g_runtime_global_context.m_input_system;
        if (!input_system)
            return;
        
        // Handle keyboard movement
        if (input_system->isKeyPressing(KeyBoardButton::W) || input_system->isKeyPressing(KeyBoardButton::UP))
            m_camera.processKeyboard(CameraMovement::FORWARD, delta_time);
        if (input_system->isKeyPressing(KeyBoardButton::S) || input_system->isKeyPressing(KeyBoardButton::DOWN))
            m_camera.processKeyboard(CameraMovement::BACKWARD, delta_time);
        if (input_system->isKeyPressing(KeyBoardButton::A) || input_system->isKeyPressing(KeyBoardButton::LEFT))
            m_camera.processKeyboard(CameraMovement::LEFT, delta_time);
        if (input_system->isKeyPressing(KeyBoardButton::D) || input_system->isKeyPressing(KeyBoardButton::RIGHT))
            m_camera.processKeyboard(CameraMovement::RIGHT, delta_time);
        if (input_system->isKeyPressing(KeyBoardButton::Q))
            m_camera.processKeyboard(CameraMovement::UP, delta_time);
        if (input_system->isKeyPressing(KeyBoardButton::E))
            m_camera.processKeyboard(CameraMovement::DOWN, delta_time);
        
        // Handle resetting camera position
        if (input_system->isKeyReleased(KeyBoardButton::SPACE)) {
            // Reset camera to default position and orientation
            m_camera.setPosition(glm::float3(0.0f, 0.0f, 5.0f));
            m_camera.setFront(glm::float3(0.0f, 0.0f, -1.0f));
            m_camera.setUp(glm::float3(0.0f, 1.0f, 0.0f));
            m_camera.setWorldUp(glm::float3(0.0f, 1.0f, 0.0f));
            m_camera.setYaw(-90.0f);
            m_camera.setPitch(0.0f);
        }
        
        // Handle speed modifiers
        float speedMultiplier = 1.0f;
        if (input_system->isKeyPressing(KeyBoardButton::LEFT_SHIFT))
            speedMultiplier = 2.0f; // Sprint
        if (input_system->isKeyPressing(KeyBoardButton::LEFT_CONTROL))
            speedMultiplier = 0.5f; // Slow walk
            
        // Apply speed multiplier
        if (speedMultiplier != 1.0f) {
            float baseSpeed = m_camera_res.m_FreeCamParam.m_speed;
            m_camera.setMovementSpeed(baseSpeed * speedMultiplier);
        }
        
        // Reset to base speed if no modifier keys are pressed
        if (speedMultiplier == 1.0f) {
            m_camera.setMovementSpeed(m_camera_res.m_FreeCamParam.m_speed);
        }
            
        // Handle mouse movement for camera rotation
        float yaw_offset = input_system->m_cursor_delta_yaw * 100.0f;
        float pitch_offset = input_system->m_cursor_delta_pitch * 100.0f;
        m_camera.processMouseMovement(yaw_offset, pitch_offset);
        
        // Handle mouse scroll for zoom
        float scroll_offset = input_system->m_cursor_delta_scroll;
        if (scroll_offset != 0.0f)
            m_camera.processMouseScroll(scroll_offset);
        
        // Send camera data to render system
        updateCameraSwapData();
    }

} // namespace MoYu
