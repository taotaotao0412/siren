#pragma once



#include <spdlog/async.h>
#include <spdlog/logger.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/spdlog.h>
#include <fmt/std.h>
#include "spdlog/sinks/rotating_file_sink.h"
#include "spdlog/sinks/stdout_color_sinks.h"

class Logger {
   public:
    static Logger &getInstance();

    spdlog::logger &getLogger();

   private:
    Logger();
    
    void useSyncLogger();

    void useASyncLogger();

    std::shared_ptr<spdlog::logger> logger_;
};

// 在任何地方使用日志记录器

#define ASYNC_LOGGER
#ifdef ASYNC_LOGGER
#define LOG_TRACE(...) Logger::getInstance().getLogger().trace(__VA_ARGS__)
#define LOG_DEBUG(...) Logger::getInstance().getLogger().debug(__VA_ARGS__)
#define LOG_INFO(...) Logger::getInstance().getLogger().info(__VA_ARGS__)
#define LOG_WARN(...) Logger::getInstance().getLogger().warn(__VA_ARGS__)
#define LOG_CRITICAL(...) \
    Logger::getInstance().getLogger().critical(__VA_ARGS__)
#define LOG_ERROR(...) Logger::getInstance().getLogger().error(__VA_ARGS__)
#else
#define LOG_TRACE SPDLOG_LOGGER_TRACE
#define LOG_DEBUG SPDLOG_LOGGER_DEBUG
#define LOG_INFO SPDLOG_LOGGER_INFO
#define LOG_WARN SPDLOG_LOGGER_WARN
#define LOG_CRITICAL SPDLOG_LOGGER_CRITICAL
#define LOG_ERROR SPDLOG_LOGGER_ERROR
#endif
// #define SPDLOG_ACTIVE_LEVEL SPDLOG_LEVEL_TRACE
