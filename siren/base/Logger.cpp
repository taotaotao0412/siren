#include "siren/base/Logger.h"


spdlog::logger& Logger::getLogger() {
    // TODO: insert return statement here
    return *logger_;
}

Logger& Logger::getInstance() {
    // TODO: insert return statement here
    static Logger instance;
    return instance;
}

Logger::Logger() {
    {
        // useSyncLogger();
        useASyncLogger();
        spdlog::set_level(spdlog::level::info);
    }
}

void Logger::useSyncLogger() {
    auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
    console_sink->set_level(spdlog::level::trace);
    // console_sink->set_pattern("[siren] [%^%l%$] [%s] [%#] %v");


    auto file_sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>(
        "logs/sync.log", true);
    file_sink->set_level(spdlog::level::trace);

    spdlog::logger logger("multi_sink", {console_sink, file_sink});
    std::vector<spdlog::sink_ptr> sinks{console_sink, file_sink};
    logger_ = std::make_shared<spdlog::logger>("siren", sinks.begin(), sinks.end());
    logger_->set_level(spdlog::level::trace);

}

void Logger::useASyncLogger() {
    auto stdout_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
    auto rotating_sink = std::make_shared<spdlog::sinks::rotating_file_sink_mt>(
        "logs/log.log", 1024 * 1024 * 10, 3);

    auto async_file = spdlog::basic_logger_mt<spdlog::async_factory>(
        "async_file_logger", "logs/async_log.txt");

    std::vector<spdlog::sink_ptr> sinks{stdout_sink, rotating_sink};
    auto logger = std::make_shared<spdlog::async_logger>(
        "defualt_logger", sinks.begin(), sinks.end(), spdlog::thread_pool(),
        spdlog::async_overflow_policy::block);
    spdlog::register_logger(logger);
    logger_ = logger;

    spdlog::set_level(spdlog::level::trace);
}