#pragma once

#include "runtime/function/render/render_common.h"
#include "runtime/function/render/render_guid_allocator.h"
#include "runtime/function/render/render_swap_context.h"

#include "runtime/resource/res_type/components/mesh_renderer.h"
#include "runtime/resource/res_type/components/volume_renderer.h"
#include "runtime/resource/res_type/components/terrain.h"

#include <array>
#include <memory>
#include <optional>

namespace MoYu
{
    class WindowSystem;
    class RenderResource;
    class RendererManager;
    class RenderScene;
    class RenderCamera;
    class WindowUI;

    struct EngineContentViewport;
    struct GlobalRenderingRes;

    struct RenderSystemInitInfo
    {
        std::shared_ptr<WindowSystem> window_system;
    };

    class RenderSystem
    {
    public:
        RenderSystem() = default;
        ~RenderSystem();

        void initialize(RenderSystemInitInfo init_info);
        void tick();

        void                          swapLogicRenderData();
        RenderSwapContext&            getSwapContext();
        std::shared_ptr<RenderCamera> getRenderCamera() const;

        void initializeUIRenderBackend(WindowUI* window_ui);
        
        EngineContentViewport getEngineContentViewport() const;

        void clearForLevelReloading();

    private:
        RenderSwapContext m_swap_context;

        std::shared_ptr<RenderCamera>    m_render_camera;
        std::shared_ptr<RenderScene>     m_render_scene;
        std::shared_ptr<RenderResource>  m_render_resource;
        std::shared_ptr<RendererManager> m_renderer_manager;

        void processSwapData(float deltaTimeMs);
        void setupRenderCamera(const GlobalRenderingRes& global_rendering_res);
        
        // 资源预加载函数
        void preloadMeshResources(const SceneMeshRenderer& meshRenderer);
        void preloadVolumeResources(const SceneVolumeFogRenderer& volumeRenderer);
        void preloadTerrainResources(const SceneTerrainRenderer& terrainRenderer);
    };
} // namespace MoYu