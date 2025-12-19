#pragma once

#include <atomic>
#include <chrono>
#include <filesystem>
#include <string>
#include <unordered_set>
#include <thread>
#include <future>
#include <memory>

namespace MoYu
{
    class ThreadPool;

    extern bool                            g_is_editor_mode;
    extern std::unordered_set<std::string> g_editor_tick_component_types;

    class PilotEngine
    {
        friend class PilotEditor;

        static const float k_fps_alpha;

    public:
        void startEngine(const std::string& config_file_path);
        void shutdownEngine();

        void initialize();
        void clear();

        bool isQuit() const { return m_is_quit; }
        void run();
        bool tickOneFrame(float delta_time);

        int getFPS() const { return m_fps; }

    protected:
        void logicalTick(float delta_time);
        bool rendererTick();

        void calculateFPS(float delta_time);

        /**
         *  Each frame can only be called once
         */
        float calculateDeltaTime();

        // Multi-threading support
        void startGameThread();
        void stopGameThread();
        void gameThreadFunc();

        bool m_is_quit {false};

        std::chrono::steady_clock::time_point m_last_tick_time_point {std::chrono::steady_clock::now()};

        float m_average_duration {0.f};
        int   m_frame_count {0};
        int   m_fps {0};

        // Threading members
        std::unique_ptr<ThreadPool> m_game_thread_pool;
        std::unique_ptr<ThreadPool> m_render_thread_pool;
        std::atomic<bool> m_game_thread_running {false};
        std::future<void> m_game_thread_future;
        std::atomic<bool> m_exit_requested {false};
    };

} // namespace MoYu