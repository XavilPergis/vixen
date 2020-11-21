#pragma once

#include "./types.hpp"

#include <spdlog/fmt/ostr.h>
#include <spdlog/logger.h>

#define XEN_LOG_FORMAT_START(Ty)               \
    template <typename S>                      \
    friend S &operator<<(S &s, const Ty &ty) { \
        return s << #Ty " { "
#define XEN_LOG_FORMAT_ITEM(item) << #item "=" << ty.item << " "
#define XEN_LOG_FORMAT_END() \
    << "}";                  \
    }

#define XEN_ENGINE_LOG(level, ...)                                                   \
    xen::engine_logger()->log(                                                       \
        spdlog::source_loc(xen::util::strip_file_path(__FILE__), __LINE__, nullptr), \
        level,                                                                       \
        __VA_ARGS__);
#define XEN_ENGINE_TRACE(...) XEN_ENGINE_LOG(spdlog::level::trace, __VA_ARGS__);
#define XEN_ENGINE_DEBUG(...) XEN_ENGINE_LOG(spdlog::level::debug, __VA_ARGS__);
#define XEN_ENGINE_INFO(...) XEN_ENGINE_LOG(spdlog::level::info, __VA_ARGS__);
#define XEN_ENGINE_WARN(...) XEN_ENGINE_LOG(spdlog::level::warn, __VA_ARGS__);
#define XEN_ENGINE_ERROR(...) XEN_ENGINE_LOG(spdlog::level::err, __VA_ARGS__);
#define XEN_ENGINE_CRITICAL(...) XEN_ENGINE_LOG(spdlog::level::critical, __VA_ARGS__);

#define XEN_LOG(level, ...)                                                          \
    xen::application_logger()->log(                                                  \
        spdlog::source_loc(xen::util::strip_file_path(__FILE__), __LINE__, nullptr), \
        level,                                                                       \
        __VA_ARGS__);
#define XEN_TRACE(...) XEN_LOG(spdlog::level::trace, __VA_ARGS__);
#define XEN_DEBUG(...) XEN_LOG(spdlog::level::debug, __VA_ARGS__);
#define XEN_INFO(...) XEN_LOG(spdlog::level::info, __VA_ARGS__);
#define XEN_WARN(...) XEN_LOG(spdlog::level::warn, __VA_ARGS__);
#define XEN_ERROR(...) XEN_LOG(spdlog::level::err, __VA_ARGS__);
#define XEN_CRITICAL(...) XEN_LOG(spdlog::level::critical, __VA_ARGS__);

namespace xen {
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

} // namespace xen
