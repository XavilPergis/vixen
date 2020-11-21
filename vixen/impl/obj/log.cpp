#include "vixen/log.hpp"

#include "vixen/types.hpp"

#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/spdlog.h>

namespace vixen {
const char *default_log_pattern = "%^[%T] [%@] [%n/%l]%$ %v";

Shared<spdlog::logger> create_logger(const char *name) noexcept {
    Shared<spdlog::logger> logger = spdlog::stdout_color_mt(name);
    logger->set_pattern(default_log_pattern);
    logger->set_level(spdlog::level::debug);
    return logger;
}

Shared<spdlog::logger> engine_logger() noexcept {
    static Shared<spdlog::logger> g_engine_logger = create_logger("vixen Engine");
    return g_engine_logger;
}

Shared<spdlog::logger> application_logger() noexcept {
    static Shared<spdlog::logger> g_application_logger = create_logger("vixen Client Application");
    return g_application_logger;
}

} // namespace vixen
