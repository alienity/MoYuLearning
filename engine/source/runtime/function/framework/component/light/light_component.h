#pragma once

#include "runtime/function/framework/component/component.h"
#include "runtime/function/framework/object/Light.h"
#include "runtime/resource/res_type/components/light.h"

#include <memory>
#include <string>

namespace MoYu
{
    class LightComponent : public Component
    {
    public:
        LightComponent() { m_component_name = "LightComponent"; };

        void reset();

        virtual void postLoadResource(std::weak_ptr<GObject> object, const std::string json_data) override;

        virtual void save(ComponentDefinitionRes& out_component_res) override;

        virtual void tick(float delta_time) override;

        // For editor
        LightComponentRes& getLightComponent() { return m_light_res_buffer[m_next_index]; }
        
        // Get the actual light object
        std::shared_ptr<Light> getLight() { return m_light; }

    private:
        bool isLightTypeInit();

        LightComponentRes m_light_res_buffer[2] {};
        uint32_t m_current_index {0};
        uint32_t m_next_index {1};
        
        // The actual light object
        std::shared_ptr<Light> m_light;
    };
} // namespace MoYu