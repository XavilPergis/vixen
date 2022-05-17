#include "vixen/log.hpp"

#include "vixen/defines.hpp"
#include "vixen/types.hpp"
#include "vixen/vector.hpp"

#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/spdlog.h>

namespace vixen {

const char *default_log_pattern_verbose
    = "\x1b[1m%@\x1b[0m \x1b[1m~\x1b[0m \x1b[1m%!\x1b[0m \x1b[1m~\x1b[0m %T \x1b[1m~\x1b[0m %^%n/%l%$ \x1b[1m>\x1b[0m %v";

const char *default_log_pattern_simple
    = "\x1b[1m%@\x1b[0m \x1b[1m~\x1b[0m %^%n/%l%$ \x1b[1m>\x1b[0m %v";

LoggerId get_default_logger() {
    static LoggerId g_default_logger = create_logger("default", [](LoggerId id) {
#ifdef VIXEN_IS_DEBUG
        set_logger_format_string(id, default_log_pattern_simple);
        // set_logger_format_string(id, default_log_pattern_simple);
        set_logger_verbosity(id, spdlog::level::debug);
#else
        set_logger_format_string(id, default_log_pattern_simple);
        set_logger_verbosity(id, spdlog::level::warn);
#endif
    });
    return g_default_logger;
}

std::shared_ptr<spdlog::logger> create_raw_logger(const char *name) noexcept {
    std::shared_ptr<spdlog::logger> logger = spdlog::stdout_color_mt(name);
#ifdef VIXEN_IS_DEBUG
    logger->set_pattern(default_log_pattern_simple);
    logger->set_level(spdlog::level::debug);
#else
    logger->set_pattern(default_log_pattern_simple);
    logger->set_level(spdlog::level::warn);
#endif
    return logger;
}

static Vector<std::shared_ptr<spdlog::logger>> &get_logger_legistry() {
    static Vector<std::shared_ptr<spdlog::logger>> gLoggers(heap::debugAllocator());
    return gLoggers;
}

LoggerId create_logger(const char *name, logger_initializer initializer) {
    get_logger_legistry().insertLast(create_raw_logger(name));
    LoggerId id{get_logger_legistry().len() - 1};
    initializer(id);
    return id;
}

void set_logger_format_string(LoggerId id, const char *fmt) {
    get_logger_legistry()[id.id]->set_pattern(fmt);
}

void set_logger_verbosity(LoggerId id, logger_level level) {
    get_logger_legistry()[id.id]->set_level(level);
}

spdlog::logger &get_raw_logger(LoggerId id) {
    std::vector<int> foo;
    return *get_logger_legistry()[id.id];
}

} // namespace vixen
