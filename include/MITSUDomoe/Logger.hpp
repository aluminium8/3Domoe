#pragma once

#include <spdlog/spdlog.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <iostream>
#include <chrono>
#include <filesystem>
#include <iomanip>
#include <sstream>

namespace MITSU_Domoe {

constexpr int LOG_ID_PADDING = 4;

inline std::filesystem::path initialize_logger() {
    try {
        // Get current time
        auto now = std::chrono::system_clock::now();
        auto in_time_t = std::chrono::system_clock::to_time_t(now);

        // Format time to yyyymmdd_hhmmss
        std::stringstream ss;
        ss << std::put_time(std::localtime(&in_time_t), "%Y%m%d_%H%M%S");
        std::string timestamp = ss.str();

        // Create directory
        std::filesystem::path result_dir = std::filesystem::path("./result") / timestamp;
        std::filesystem::create_directories(result_dir);

        // Create log file path
        std::filesystem::path log_file = result_dir / "log.txt";

        // Create sinks
        auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
        console_sink->set_level(spdlog::level::info);

        auto file_sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>(log_file.string(), true);
        file_sink->set_level(spdlog::level::trace);

        // Create logger
        std::vector<spdlog::sink_ptr> sinks{console_sink, file_sink};
        auto logger = std::make_shared<spdlog::logger>("mitsudomoe_logger", sinks.begin(), sinks.end());
        logger->set_level(spdlog::level::trace);
        logger->flush_on(spdlog::level::trace);

        // Register logger
        spdlog::register_logger(logger);
        spdlog::set_default_logger(logger);

        spdlog::info("Logger initialized. Log files will be saved to: {}", result_dir.string());
        return result_dir;
    } catch (const spdlog::spdlog_ex& ex) {
        std::cout << "Log initialization failed: " << ex.what() << std::endl;
        return "";
    }
}

} // namespace MITSU_Domoe
