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

        virtual void reset();

        virtual void postLoadResource(std::weak_ptr<GObject> object, const std::string json_data) override;

        virtual void save(ComponentDefinitionRes& out_component_res) override;

        virtual void tick(float delta_time) override;

        // for editor
        virtual CameraComponentRes& getCameraComponent() { return m_camera_res; }

    private:
        virtual void tickFirstPersonCamera(float delta_time);
        virtual void tickThirdPersonCamera(float delta_time);
        virtual void tickFreeCamera(float delta_time);
        virtual void updateCameraSwapData();

        CameraComponentRes m_camera_res;

        EditorCamera m_camera;
    };
} // namespace MoYu