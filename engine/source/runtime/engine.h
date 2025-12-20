#pragma once

#include <atomic>
#include <chrono>
#include <filesystem>
#include <string>
#include <unordered_set>
#include <thread>
#include <future>
#include <memory>
#include <functional>
#include "function/thread/thread_pool.h"

namespace MoYu
{
    extern bool g_is_editor_mode;
    extern std::unordered_set<std::string> g_editor_tick_component_types;

    // Define main loop delegate type
    using MainLoopDelegate = std::function<void(float delta_time)>;

    // Define render delegate type
    using RenderDelegate = std::function<void()>;

    class MoYuEngine
    {
        friend class MoYuEditor;

        static const float k_fps_alpha;

    public:
        void startEngine(const std::string& config_file_path);
        void shutdownEngine();

        void initialize();
        void clear();

        bool isQuit() const { return m_is_quit; }
        void run();
        
        int getFPS() const { return m_fps; }

        // Set main loop delegate
        void setMainLoopDelegate(MainLoopDelegate delegate) { m_main_loop_delegate = std::move(delegate); }
        // Set render delegate
        void setRenderDelegate(RenderDelegate delegate) { m_render_delegate = std::move(delegate); }

    protected:
        void logicalTick(float delta_time);
        bool rendererTick();

        void calculateFPS(float delta_time);

        /**
         *  Each frame can only be called once
         */
        float calculateDeltaTime();

        // Multi-threading support
        void startRenderThread();
        void stopRenderThread();
        void renderThreadFunc();

        bool m_is_quit {false};

        std::chrono::steady_clock::time_point m_last_tick_time_point {std::chrono::steady_clock::now()};

        float m_average_duration {0.f};
        int   m_frame_count {0};
        int   m_fps {0};

        // Threading members
        std::unique_ptr<ThreadPool> m_render_thread_pool;
        std::atomic<bool> m_render_thread_running {false};
        std::future<void> m_render_thread_future;
        std::atomic<bool> m_exit_requested {false};
        std::mutex m_render_mutex;
        std::condition_variable m_render_cv;
        bool m_frame_ready {false};
        bool m_frame_processed {true};

        // Main loop delegate
        MainLoopDelegate m_main_loop_delegate;
        // Render delegate
        RenderDelegate m_render_delegate;
    };

} // namespace MoYu