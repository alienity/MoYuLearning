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

    void MoYuEngine::startEngine(const std::string& config_file_path)
    {
        g_runtime_global_context.startSystems(config_file_path);

        LOG_INFO("engine start");
    }

    void MoYuEngine::shutdownEngine()
    {
        LOG_INFO("engine shutdown");

        // Stop render thread before shutting down systems
        stopRenderThread();

        g_runtime_global_context.shutdownSystems();
    }

    void MoYuEngine::initialize() {}
    void MoYuEngine::clear() {}

    void MoYuEngine::run()
    {
        // Initialize render thread pool
        m_render_thread_pool = std::make_unique<ThreadPool>(1);

        // Start render thread
        startRenderThread();

        std::shared_ptr<WindowSystem> window_system = g_runtime_global_context.m_window_system;
        ASSERT(window_system);

        while (!window_system->shouldClose() && !m_exit_requested.load())
        {
            // Poll window events on main thread
            g_runtime_global_context.m_window_system->pollEvents();
            
            // Execute main loop logic
            const float delta_time = calculateDeltaTime();
            
            // If main loop delegate is set, call it (e.g. execute editor logic)
            if (m_main_loop_delegate)
            {
                m_main_loop_delegate(delta_time);
            }

            // Process game logic on main thread
            logicalTick(delta_time);

            // Notify render thread that a new frame is ready
            {
                std::lock_guard<std::mutex> lock(m_render_mutex);
                m_frame_ready = true;
                m_frame_processed = false;
            }
            m_render_cv.notify_one();
            
            // Wait for render thread to finish processing the frame
            {
                std::unique_lock<std::mutex> lock(m_render_mutex);
                m_render_cv.wait(lock, [this] { return m_frame_processed && !m_frame_ready; });
            }
            
            // Update window title
            g_runtime_global_context.m_window_system->setTile(
                std::string("MoYu - " + std::to_string(getFPS()) + " FPS").c_str());

            calculateFPS(delta_time);
        }

        // Stop threads before exiting
        stopRenderThread();
    }

    void MoYuEngine::startRenderThread()
    {
        m_render_thread_running.store(true);
        m_render_thread_future = m_render_thread_pool->enqueue([this]() {
            renderThreadFunc();
        });
    }

    void MoYuEngine::stopRenderThread()
    {
        m_render_thread_running.store(false);
        m_exit_requested.store(true);
        
        // Wake up render thread so it can exit
        {
            std::lock_guard<std::mutex> lock(m_render_mutex);
            m_frame_ready = true;
        }
        m_render_cv.notify_one();
        
        if (m_render_thread_future.valid()) {
            m_render_thread_future.wait();
        }
        
        if (m_render_thread_pool) {
            m_render_thread_pool->stop();
        }
    }

    void MoYuEngine::renderThreadFunc()
    {
        // Render thread main loop
        while (m_render_thread_running.load() && !m_exit_requested.load())
        {
            // Wait for a new frame to be ready
            std::unique_lock<std::mutex> lock(m_render_mutex);
            m_render_cv.wait(lock, [this] { return m_frame_ready; });
            
            if (!m_render_thread_running.load() || m_exit_requested.load())
                break;
            
            // Process render commands
            // Exchange data between logic and render contexts
            g_runtime_global_context.m_render_system->swapLogicRenderData();
            
            // If render delegate is set, call it (e.g. execute editor render logic)
            if (m_render_delegate)
            {
                m_render_delegate();
            }

            rendererTick();
            // Mark frame as processed
            m_frame_ready = false;
            m_frame_processed = true;
            lock.unlock();
            m_render_cv.notify_one();
        }
    }

    float MoYuEngine::calculateDeltaTime()
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

    void MoYuEngine::logicalTick(float delta_time)
    {
        g_runtime_global_context.m_world_manager->tick(delta_time);
        g_runtime_global_context.m_input_system->tick();
        g_runtime_global_context.m_material_manager->tick(delta_time);
    }

    bool MoYuEngine::rendererTick()
    {
        g_runtime_global_context.m_render_system->tick();
        return true;
    }

    const float MoYuEngine::k_fps_alpha = 1.f / 100;

    void MoYuEngine::calculateFPS(float delta_time)
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