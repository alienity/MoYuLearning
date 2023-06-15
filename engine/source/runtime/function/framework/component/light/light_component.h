#pragma once

#include "runtime/core/math/moyu_math.h"
#include "runtime/resource/res_type/components/light.h"
#include "runtime/function/framework/component/component.h"

namespace MoYu
{
    class RenderCamera;

    class LightComponent : public Component
    {
    public:
        LightComponent() = default;

        void reset();

        void postLoadResource(std::weak_ptr<GObject> object, void* data) override;

        void tick(float delta_time) override;

    //private:
        // Editorֱ�ӱ༭�������Ϊ��ǰ֡���޸Ķ����޸ĺ�һ��Ҫ����SetDirtyFlag
        LightComponentRes m_light_res;

        // ����1�ǵ�ǰ֡��2����һ֡
        // 1�ǿգ�2����Light�ģ���ӹ�Դ
        // 1����Light�ģ�2�ǿգ�ɾ����Դ
        // 1��2����Light��������ͬ�����¹�Դ
        LightComponentRes m_light_res_buffer[2];
        uint32_t m_current_index {0};
        uint32_t m_next_index {1};
    };
} // namespace MoYu
