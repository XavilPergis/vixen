#include "vixen/log.hpp"

#include "vixen/defines.hpp"
#include "vixen/types.hpp"
#include "vixen/vec.hpp"

#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/spdlog.h>

namespace vixen {

const char *default_log_pattern_verbose
    = "\x1b[1m%@\x1b[0m \x1b[1m~\x1b[0m \x1b[1m%!\x1b[0m \x1b[1m~\x1b[0m %T \x1b[1m~\x1b[0m %^%n/%l%$ \x1b[1m>\x1b[0m %v";

const char *default_log_pattern_simple
    = "\x1b[1m%@\x1b[0m \x1b[1m~\x1b[0m %^%n/%l%$ \x1b[1m>\x1b[0m %v";

logger_id default_logger = create_logger("default", [](logger_id id) {
#ifdef VIXEN_IS_DEBUG
    set_logger_format_string(id, default_log_pattern_simple);
    // set_logger_format_string(id, default_log_pattern_simple);
    set_logger_verbosity(id, spdlog::level::debug);
#else
    set_logger_format_string(id, default_log_pattern_simple);
    set_logger_verbosity(id, spdlog::level::warn);
#endif
});

std::shared_ptr<spdlog::logger> create_raw_logger(const char *name) noexcept {
    std::shared_ptr<spdlog::logger> logger = spdlog::stdout_color_mt(name);
#ifdef VIXEN_IS_DEBUG
    logger->set_pattern(default_log_pattern_verbose);
    logger->set_level(spdlog::level::debug);
#else
    logger->set_pattern(default_log_pattern_simple);
    logger->set_level(spdlog::level::warn);
#endif
    return logger;
}

static vector<std::shared_ptr<spdlog::logger>> &get_logger_legistry() {
    static vector<std::shared_ptr<spdlog::logger>> g_loggers(heap::debug_allocator());
    return g_loggers;
}

logger_id create_logger(const char *name, logger_initializer initializer) {
    get_logger_legistry().push(create_raw_logger(name));
    logger_id id{get_logger_legistry().len() - 1};
    initializer(id);
    return id;
}

void set_logger_format_string(logger_id id, const char *fmt) {
    get_logger_legistry()[id.id]->set_pattern(fmt);
}

void set_logger_verbosity(logger_id id, logger_level level) {
    get_logger_legistry()[id.id]->set_level(level);
}

spdlog::logger &get_raw_logger(logger_id id) {
    return *get_logger_legistry()[id.id];
}

} // namespace vixen
