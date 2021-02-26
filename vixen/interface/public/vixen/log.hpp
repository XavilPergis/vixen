#pragma once

#include "vixen/types.hpp"

#include <spdlog/fmt/ostr.h>
#include <spdlog/logger.h>

#ifdef __GNUC__
#define VIXEN_PRETTY_FUNCTION __PRETTY_FUNCTION__
#else
#define VIXEN_PRETTY_FUNCTION __func__
#endif

#define VIXEN_LOG_FORMAT_START(Ty)             \
    template <typename S>                      \
    friend S &operator<<(S &s, const Ty &ty) { \
        return s << #Ty " { "
#define VIXEN_LOG_FORMAT_ITEM(item) << #item "=" << ty.item << " "
#define VIXEN_LOG_FORMAT_END() \
    << "}";                    \
    }

#define VIXEN_FORMAT_DECL(Ty) \
    template <typename S>     \
    friend S &operator<<(S &s, const Ty &ty);

#define _VIXEN_CAPTURE_SOURCE_LOCATION()                                          \
    ::vixen::source_location {                                                    \
        ::vixen::util::strip_file_path(__FILE__), __LINE__, VIXEN_PRETTY_FUNCTION \
    }

#define VIXEN_LOG(level, ...)                   \
    ::vixen::log(::vixen::get_default_logger(), \
        _VIXEN_CAPTURE_SOURCE_LOCATION(),       \
        level,                                  \
        __VA_ARGS__);
#define VIXEN_TRACE(...) VIXEN_LOG(::vixen::logger_level::trace, __VA_ARGS__);
#define VIXEN_DEBUG(...) VIXEN_LOG(::vixen::logger_level::debug, __VA_ARGS__);
#define VIXEN_INFO(...) VIXEN_LOG(::vixen::logger_level::info, __VA_ARGS__);
#define VIXEN_WARN(...) VIXEN_LOG(::vixen::logger_level::warn, __VA_ARGS__);
#define VIXEN_ERROR(...) VIXEN_LOG(::vixen::logger_level::err, __VA_ARGS__);
#define VIXEN_CRITICAL(...) VIXEN_LOG(::vixen::logger_level::critical, __VA_ARGS__);

#define VIXEN_LOG_EXT(logger_id, level, ...) \
    ::vixen::log(logger_id, _VIXEN_CAPTURE_SOURCE_LOCATION(), level, __VA_ARGS__);
#define VIXEN_TRACE_EXT(logger_id, ...) \
    VIXEN_LOG_EXT(logger_id, ::vixen::logger_level::trace, __VA_ARGS__);
#define VIXEN_DEBUG_EXT(logger_id, ...) \
    VIXEN_LOG_EXT(logger_id, ::vixen::logger_level::debug, __VA_ARGS__);
#define VIXEN_INFO_EXT(logger_id, ...) \
    VIXEN_LOG_EXT(logger_id, ::vixen::logger_level::info, __VA_ARGS__);
#define VIXEN_WARN_EXT(logger_id, ...) \
    VIXEN_LOG_EXT(logger_id, ::vixen::logger_level::warn, __VA_ARGS__);
#define VIXEN_ERROR_EXT(logger_id, ...) \
    VIXEN_LOG_EXT(logger_id, ::vixen::logger_level::err, __VA_ARGS__);
#define VIXEN_CRITICAL_EXT(logger_id, ...) \
    VIXEN_LOG_EXT(logger_id, ::vixen::logger_level::critical, __VA_ARGS__);

namespace vixen {

struct logger_id {
    usize id;
};

// Sorta weird API is due to wanting to be able to register loggers automatically during static
// initialization.
using logger_initializer = void (*)(logger_id);

using logger_level = ::spdlog::level::level_enum;
using source_location = ::spdlog::source_loc;

logger_id create_logger(const char *name, logger_initializer initializer);

void set_logger_format_string(logger_id id, const char *fmt);
void set_logger_verbosity(logger_id id, logger_level level);

spdlog::logger &get_raw_logger(logger_id id);

template <typename... Args>
void log(logger_id logger, source_location loc, logger_level lvl, const char *fmt, Args &&...args) {
    get_raw_logger(logger).log(loc, lvl, fmt, std::forward<Args>(args)...);
}

logger_id get_default_logger();

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
