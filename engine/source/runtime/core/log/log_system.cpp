#include "runtime/core/log/log_system.h"
#include "runtime/function/render/rhi/d3d12/d3d12_linkedDevice.h"

#include <spdlog/async.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/pattern_formatter.h>
#include <spdlog/spdlog.h>
#include <filesystem>
#include <iomanip>
#include <sstream>
#include <chrono>
#include <iostream>

namespace MoYu
{
    std::string get_log_file_path() {
        namespace fs = std::filesystem;
        using namespace std::chrono;

        fs::path log_dir = "logs";

        auto now = system_clock::now();
        auto in_time_t = system_clock::to_time_t(now);

        std::tm tm;
#ifdef _WIN32
        localtime_s(&tm, &in_time_t);
#else
        localtime_r(&in_time_t, &tm);
#endif

        std::ostringstream date_ss;
        date_ss << std::put_time(&tm, "%Y%m%d");
        log_dir /= date_ss.str();

        fs::create_directories(log_dir);

        auto ms = duration_cast<milliseconds>(now.time_since_epoch()) % 1000;
        std::ostringstream file_ss;
        file_ss << "app_"
            << std::put_time(&tm, "%H%M%S")
            << '_' << std::setfill('0') << std::setw(3) << ms.count()
            << ".log";

        return (log_dir / file_ss.str()).string();
    }

    long GetLogFrameIndex()
    {
        return RHI::D3D12LinkedDevice::m_FrameIndex;
    }

    LogSystem* m_LogSystem;

    LogSystem::LogSystem()
    {
        try
        {
            auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
            console_sink->set_level(spdlog::level::trace);
            console_sink->set_pattern("%^[%Y-%m-%d %H:%M:%S.%e] [%l] [thread %t] %v%$");

            std::string log_path = get_log_file_path();
            auto file_sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>(log_path, true);
            file_sink->set_level(spdlog::level::trace);
            file_sink->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%l] [thread %t] %v");

            const spdlog::sinks_init_list sink_list = { console_sink, file_sink };

            spdlog::init_thread_pool(8192, 1);

            m_logger = std::make_shared<spdlog::async_logger>("muggle_logger",
                sink_list.begin(),
                sink_list.end(),
                spdlog::thread_pool(),
                spdlog::async_overflow_policy::block);

            m_logger->set_level(spdlog::level::trace);

            spdlog::register_logger(m_logger);

            spdlog::flush_every(std::chrono::seconds(5));

            spdlog::info("Logging started. File: {}", log_path);
            spdlog::debug("Debug messages enabled");
            spdlog::warn("This is a warning");

            spdlog::flush_on(spdlog::level::err);
        }
        catch (const spdlog::spdlog_ex& ex)
        {
            std::cerr << "Log initialization failed: " << ex.what() << std::endl;
        }
    }

    LogSystem* LogSystem::Instance()
    {
        if (m_LogSystem == nullptr)
            m_LogSystem = new LogSystem();
        return m_LogSystem;
    }

    LogSystem::~LogSystem()
    {
        spdlog::info("Application exiting");

        m_logger->flush();
        spdlog::drop_all();

        spdlog::shutdown();

        delete m_LogSystem;
        m_LogSystem = nullptr;
    }

} // namespace MoYu
