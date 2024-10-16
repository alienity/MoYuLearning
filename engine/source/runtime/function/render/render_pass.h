#pragma once

#include "runtime/function/render/render_common.h"
#include "runtime/function/render/render_pass_base.h"
#include "runtime/function/render/render_resource.h"
#include "runtime/function/render/renderer/renderer_registry.h"

#include <memory>
#include <vector>

namespace MoYu
{
    class RenderScene;
    class RenderCamera;

    class RenderPass : public RenderPassBase
    {
    public:
        static RenderScene* m_render_scene;
        static RenderCamera* m_render_camera;

    private:
    };
} // namespace MoYu
