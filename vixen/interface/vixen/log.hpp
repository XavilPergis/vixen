#pragma once

#include "./types.hpp"

#include <spdlog/fmt/ostr.h>
#include <spdlog/logger.h>

#define VIXEN_LOG_FORMAT_START(Ty)             \
    template <typename S>                      \
    friend S &operator<<(S &s, const Ty &ty) { \
        return s << #Ty " { "
#define VIXEN_LOG_FORMAT_ITEM(item) << #item "=" << ty.item << " "
#define VIXEN_LOG_FORMAT_END() \
    << "}";                    \
    }

// TODO: add function name to log info
#define VIXEN_ENGINE_LOG(level, ...)                                                   \
    vixen::engine_logger()->log(                                                       \
        spdlog::source_loc(vixen::util::strip_file_path(__FILE__), __LINE__, nullptr), \
        level,                                                                         \
        __VA_ARGS__);
#define VIXEN_ENGINE_TRACE(...) VIXEN_ENGINE_LOG(spdlog::level::trace, __VA_ARGS__);
#define VIXEN_ENGINE_DEBUG(...) VIXEN_ENGINE_LOG(spdlog::level::debug, __VA_ARGS__);
#define VIXEN_ENGINE_INFO(...) VIXEN_ENGINE_LOG(spdlog::level::info, __VA_ARGS__);
#define VIXEN_ENGINE_WARN(...) VIXEN_ENGINE_LOG(spdlog::level::warn, __VA_ARGS__);
#define VIXEN_ENGINE_ERROR(...) VIXEN_ENGINE_LOG(spdlog::level::err, __VA_ARGS__);
#define VIXEN_ENGINE_CRITICAL(...) VIXEN_ENGINE_LOG(spdlog::level::critical, __VA_ARGS__);

#define VIXEN_LOG(level, ...)                                                          \
    vixen::application_logger()->log(                                                  \
        spdlog::source_loc(vixen::util::strip_file_path(__FILE__), __LINE__, nullptr), \
        level,                                                                         \
        __VA_ARGS__);
#define VIXEN_TRACE(...) VIXEN_LOG(spdlog::level::trace, __VA_ARGS__);
#define VIXEN_DEBUG(...) VIXEN_LOG(spdlog::level::debug, __VA_ARGS__);
#define VIXEN_INFO(...) VIXEN_LOG(spdlog::level::info, __VA_ARGS__);
#define VIXEN_WARN(...) VIXEN_LOG(spdlog::level::warn, __VA_ARGS__);
#define VIXEN_ERROR(...) VIXEN_LOG(spdlog::level::err, __VA_ARGS__);
#define VIXEN_CRITICAL(...) VIXEN_LOG(spdlog::level::critical, __VA_ARGS__);

namespace vixen {
Shared<spdlog::logger> engine_logger() noexcept;
Shared<spdlog::logger> application_logger() noexcept;

namespace util {
constexpr const char *strip_file_path(const char *path) {
    const char *file = path;
    while (*path)
        if (*path++ == '/')
            file = path;
    return file;
}

} // namespace util

} // namespace vixen
