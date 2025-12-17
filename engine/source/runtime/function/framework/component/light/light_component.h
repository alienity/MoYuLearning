#pragma once

#include "runtime/core/math/moyu_math2.h"
#include "runtime/resource/res_type/components/light.h"
#include "runtime/function/framework/component/component.h"

namespace MoYu
{

    class LightComponent : public Component
    {
    public:
        LightComponent() { m_component_name = "LightComponent"; };

        void reset();

        void postLoadResource(std::weak_ptr<GObject> object, const std::string json_data) override;

        void save(ComponentDefinitionRes& out_component_res) override;

        void tick(float delta_time) override;

        // For editor
        LightComponentRes& getLightComponent() { return m_light_res_buffer[m_next_index]; }

    private:
        //// Directly edited by editor, used as modification target for current frame. Must call SetDirtyFlag after modification.
        //LightComponentRes m_light_res;
        
        bool isLightTypeInit();

        // Assume 1 is the current frame, 2 is the next frame
        // 1 is empty, 2 has Light, add light source
        // 1 has Light, 2 is empty, remove light source
        // Both 1 and 2 have Light with different parameters, update light source
        LightComponentRes m_light_res_buffer[2] {};
        uint32_t m_current_index {0};
        uint32_t m_next_index {1};
    };
} // namespace MoYu
