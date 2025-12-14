#pragma once

#include "runtime/core/math/moyu_math2.h"
#include "runtime/resource/res_type/components/camera.h"
#include "runtime/function/framework/component/component.h"
#include "runtime/function/framework/object/EditorCamera.h"

namespace MoYu
{
    class CameraComponent : public Component
    {
    public:
        CameraComponent() { m_component_name = "CameraComponent"; };

        void reset();

        void postLoadResource(std::weak_ptr<GObject> object, const std::string json_data) override;

        void save(ComponentDefinitionRes& out_component_res) override;

        void tick(float delta_time) override;

        // for editor
        CameraComponentRes& getCameraComponent() { return m_camera_res; }

    private:
        void tickFirstPersonCamera(float delta_time);
        void tickThirdPersonCamera(float delta_time);
        void tickFreeCamera(float delta_time);

        CameraComponentRes m_camera_res;

        EditorCamera m_camera;
    };
} // namespace MoYu
