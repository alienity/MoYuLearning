#include "runtime/engine.h"

#include "runtime/core/base/macro.h"
#include "runtime/function/thread/thread_pool.h"

#include "runtime/function/framework/world/world_manager.h"
#include "runtime/function/framework/material/material_manager.h"
#include "runtime/function/global/global_context.h"
#include "runtime/function/input/input_system.h"
#include "runtime/function/render/render_system.h"
#include "runtime/function/render/window_system.h"


namespace MoYu
{
    bool                            g_is_editor_mode {false};
    std::unordered_set<std::string> g_editor_tick_component_types {};

    void PilotEngine::startEngine(const std::string& config_file_path)
    {
        g_runtime_global_context.startSystems(config_file_path);

        LOG_INFO("engine start");
    }

    void PilotEngine::shutdownEngine()
    {
        LOG_INFO("engine shutdown");

        // Stop game thread before shutting down systems
        stopGameThread();

        g_runtime_global_context.shutdownSystems();
    }

    void PilotEngine::initialize() {}
    void PilotEngine::clear() {}

    void PilotEngine::run()
    {
        // Initialize thread pools
        m_game_thread_pool = std::make_unique<ThreadPool>(1);
        m_render_thread_pool = std::make_unique<ThreadPool>(1);

        // Start game thread
        startGameThread();

        std::shared_ptr<WindowSystem> window_system = g_runtime_global_context.m_window_system;
        ASSERT(window_system);

        while (!window_system->shouldClose() && !m_exit_requested.load())
        {
            const float delta_time = calculateDeltaTime();
            
            // Process render tasks in render thread
            auto render_future = m_render_thread_pool->enqueue([this, delta_time]() {
                // Exchange data between logic and render contexts
                g_runtime_global_context.m_render_system->swapLogicRenderData();
                
                rendererTick();
                
                g_runtime_global_context.m_window_system->pollEvents();
            });
            
            // Wait for render to complete
            render_future.wait();
            
            // Update window title
            g_runtime_global_context.m_window_system->setTile(
                std::string("MoYu - " + std::to_string(getFPS()) + " FPS").c_str());

            calculateFPS(delta_time);
        }

        // Stop threads before exiting
        stopGameThread();
    }

    void PilotEngine::startGameThread()
    {
        m_game_thread_running.store(true);
        m_game_thread_future = m_game_thread_pool->enqueue([this]() {
            gameThreadFunc();
        });
    }

    void PilotEngine::stopGameThread()
    {
        m_game_thread_running.store(false);
        m_exit_requested.store(true);
        
        if (m_game_thread_future.valid()) {
            m_game_thread_future.wait();
        }
        
        if (m_game_thread_pool) {
            m_game_thread_pool->stop();
        }
        
        if (m_render_thread_pool) {
            m_render_thread_pool->stop();
        }
    }

    void PilotEngine::gameThreadFunc()
    {
        // Game thread main loop
        while (m_game_thread_running.load() && !m_exit_requested.load())
        {
            const float delta_time = calculateDeltaTime();
            logicalTick(delta_time);
            
            // Sleep briefly to prevent excessive CPU usage
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
    }

    float PilotEngine::calculateDeltaTime()
    {
        float delta_time;
        {
            using namespace std::chrono;

            steady_clock::time_point tick_time_point = steady_clock::now();
            duration<float> time_span = duration_cast<duration<float>>(tick_time_point - m_last_tick_time_point);
            delta_time                = time_span.count();

            m_last_tick_time_point = tick_time_point;
        }
        return delta_time;
    }

    bool PilotEngine::tickOneFrame(float delta_time)
    {
        logicalTick(delta_time);
        calculateFPS(delta_time);

        // single thread
        // exchange data between logic and render contexts
        g_runtime_global_context.m_render_system->swapLogicRenderData();

        rendererTick();

        g_runtime_global_context.m_window_system->pollEvents();


        g_runtime_global_context.m_window_system->setTile(
            std::string("MoYu - " + std::to_string(getFPS()) + " FPS").c_str());

        const bool should_window_close = g_runtime_global_context.m_window_system->shouldClose();
        return !should_window_close;
    }

    void PilotEngine::logicalTick(float delta_time)
    {
        g_runtime_global_context.m_world_manager->tick(delta_time);
        g_runtime_global_context.m_input_system->tick();
        g_runtime_global_context.m_material_manager->tick(delta_time);
    }

    bool PilotEngine::rendererTick()
    {
        g_runtime_global_context.m_render_system->tick();
        return true;
    }

    const float PilotEngine::k_fps_alpha = 1.f / 100;
    void        PilotEngine::calculateFPS(float delta_time)
    {
        m_frame_count++;

        if (m_frame_count == 1)
        {
            m_average_duration = delta_time;
        }
        else
        {
            m_average_duration = m_average_duration * (1 - k_fps_alpha) + delta_time * k_fps_alpha;
        }

        m_fps = static_cast<int>(1.f / m_average_duration);
    }
} // namespace MoYu