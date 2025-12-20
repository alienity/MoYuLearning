#include "editor//include/editor.h"

#include "runtime/engine.h"
#include "runtime/function/global/global_context.h"
#include "runtime/function/render/render_camera.h"
#include "runtime/function/render/render_system.h"

#include "editor/include/editor_global_context.h"
#include "editor/include/editor_input_manager.h"
#include "editor/include/editor_scene_manager.h"
#include "editor/include/editor_ui.h"

namespace MoYu
{
    void registerEdtorTickComponent(std::string component_type_name)
    {
        g_editor_tick_component_types.insert(component_type_name);
    }

    MoYuEditor::MoYuEditor()
    {
        registerEdtorTickComponent("TransformComponent");
        registerEdtorTickComponent("MeshRendererComponent");
        registerEdtorTickComponent("LocalVolumetricFogComponent");
        registerEdtorTickComponent("LightComponent");
        registerEdtorTickComponent("TerrainComponent");
    }

    MoYuEditor::~MoYuEditor() {}

    void MoYuEditor::initialize(MoYuEngine* engine_runtime)
    {
        assert(engine_runtime);

        g_is_editor_mode = true;
        m_engine_runtime = engine_runtime;

        auto& window_system = g_runtime_global_context.m_window_system;
        auto& render_system = g_runtime_global_context.m_render_system;

        EditorGlobalContextInitInfo init_info = { window_system.get(), render_system.get(), engine_runtime};
        g_editor_global_context.initialize(init_info);
        g_editor_global_context.m_scene_manager->setEditorCamera(render_system->getRenderCamera());
        
        m_editor_ui = std::make_shared<EditorUI>();
        WindowUIInitInfo ui_init_info = { window_system, render_system };
        m_editor_ui->initialize(ui_init_info);

        engine_runtime->setMainLoopDelegate([this](float delta_time) { logicalTick(delta_time); });
        engine_runtime->setRenderDelegate([this]() { return rendererTick(); });
    }

    void MoYuEditor::clear() { g_editor_global_context.clear(); }

    void MoYuEditor::logicalTick(float delta_time)
    {
        assert(m_engine_runtime);
        assert(m_editor_ui);

        g_editor_global_context.m_scene_manager->tick(delta_time);
        g_editor_global_context.m_input_manager->tick(delta_time);
    }

    void MoYuEditor::rendererTick()
    {
        m_editor_ui->preRender();
    }

} // namespace MoYu
