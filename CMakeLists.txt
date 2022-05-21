cmake_minimum_required(VERSION 3.15.2)
project(vixen
    VERSION 0.1.0
    LANGUAGES CXX
)

# ## require out-of-source builds
set(VIXEN_BUILD_DIR_NAME "build")

if(NOT "${CMAKE_SOURCE_DIR}/${VIXEN_BUILD_DIR_NAME}" STREQUAL "${CMAKE_BINARY_DIR}")
    message(FATAL_ERROR "this project must be built out-of-source, in a directory named '${VIXEN_BUILD_DIR_NAME}'")
endif()

add_subdirectory(lib/spdlog)
add_subdirectory(examples)

set(SRC_PREFIX "${CMAKE_CURRENT_SOURCE_DIR}/src")
set(VIXEN_SOURCE_FILES
    "${SRC_PREFIX}/vixen/stacktrace/stacktrace.cpp"
    "${SRC_PREFIX}/vixen/stacktrace/platform.cpp"
    "${SRC_PREFIX}/vixen/allocator.cpp"
    "${SRC_PREFIX}/vixen/arena_alloc.cpp"
    "${SRC_PREFIX}/vixen/assert.cpp"
    "${SRC_PREFIX}/vixen/legacy_adapter.cpp"
    "${SRC_PREFIX}/vixen/linear_alloc.cpp"
    "${SRC_PREFIX}/vixen/log.cpp"
    "${SRC_PREFIX}/vixen/page_alloc.cpp"
    "${SRC_PREFIX}/vixen/string.cpp"
    "${SRC_PREFIX}/vixen/system_alloc.cpp"
)
set(VIXENTEST_SOURCE_FILES
    "${SRC_PREFIX}/vixentest/test.cpp"
)

set(PUB_INCLUDE_PREFIX "${CMAKE_CURRENT_SOURCE_DIR}/interface/public")
set(VIXEN_PUBLIC_HEADER_FILES
    "${PUB_INCLUDE_PREFIX}/vixen/allocator/allocator.hpp"
    "${PUB_INCLUDE_PREFIX}/vixen/allocator/allocators.hpp"
    "${PUB_INCLUDE_PREFIX}/vixen/allocator/layout.hpp"
    "${PUB_INCLUDE_PREFIX}/vixen/allocator/pool_allocator.hpp"
    "${PUB_INCLUDE_PREFIX}/vixen/allocator/profile.hpp"
    "${PUB_INCLUDE_PREFIX}/vixen/allocator/stacktrace.hpp"
    "${PUB_INCLUDE_PREFIX}/vixen/io/file_stream.hpp"
    "${PUB_INCLUDE_PREFIX}/vixen/io/file.hpp"
    "${PUB_INCLUDE_PREFIX}/vixen/any.hpp"
    "${PUB_INCLUDE_PREFIX}/vixen/assert.hpp"
    "${PUB_INCLUDE_PREFIX}/vixen/common.hpp"
    "${PUB_INCLUDE_PREFIX}/vixen/defer.hpp"
    "${PUB_INCLUDE_PREFIX}/vixen/defines.hpp"
    "${PUB_INCLUDE_PREFIX}/vixen/function.hpp"
    "${PUB_INCLUDE_PREFIX}/vixen/log.hpp"
    "${PUB_INCLUDE_PREFIX}/vixen/option.hpp"
    "${PUB_INCLUDE_PREFIX}/vixen/prof.hpp"
    "${PUB_INCLUDE_PREFIX}/vixen/result.hpp"
    "${PUB_INCLUDE_PREFIX}/vixen/shared.hpp"
    "${PUB_INCLUDE_PREFIX}/vixen/slice.hpp"
    "${PUB_INCLUDE_PREFIX}/vixen/string.hpp"
    "${PUB_INCLUDE_PREFIX}/vixen/traits.hpp"
    "${PUB_INCLUDE_PREFIX}/vixen/tuple.hpp"
    "${PUB_INCLUDE_PREFIX}/vixen/typeops.hpp"
    "${PUB_INCLUDE_PREFIX}/vixen/types.hpp"
    "${PUB_INCLUDE_PREFIX}/vixen/unique.hpp"
    "${PUB_INCLUDE_PREFIX}/vixen/util.hpp"
    "${PUB_INCLUDE_PREFIX}/vixen/vector.hpp"
)
set(VIXENTEST_PUBLIC_HEADER_FILES
    "${PUB_INCLUDE_PREFIX}/vixentest/test.hpp"
)

set(PRIV_INCLUDE_PREFIX "${CMAKE_CURRENT_SOURCE_DIR}/interface/internal")
set(VIXEN_PRIVATE_HEADER_FILES
    "${PRIV_INCLUDE_PREFIX}/vixen/hash/hasher.hpp"
    "${PRIV_INCLUDE_PREFIX}/vixen/hash/map.hpp"
    "${PRIV_INCLUDE_PREFIX}/vixen/hash/set.hpp"
    "${PRIV_INCLUDE_PREFIX}/vixen/hash/table.hpp"
)

add_library(vixen STATIC
    ${VIXEN_SOURCE_FILES}
    ${VIXEN_PUBLIC_HEADER_FILES}
    ${VIXEN_PRIVATE_HEADER_FILES}
)

add_library(vixentest STATIC
    ${VIXEN_SOURCE_FILES}
    ${VIXEN_PUBLIC_HEADER_FILES}
    ${VIXEN_PRIVATE_HEADER_FILES}
)

target_include_directories(vixen PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/interface/public")
target_include_directories(vixen PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/interface/internal")
target_include_directories(vixen PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/src/vixen")

target_include_directories(vixentest PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/interface/public")
target_include_directories(vixentest PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/interface/internal")
target_include_directories(vixentest PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/src/vixentest")

target_compile_features(vixen PRIVATE cxx_std_20)
target_compile_features(vixentest PRIVATE cxx_std_20)

target_link_libraries(vixen PUBLIC spdlog)
target_link_libraries(vixentest PUBLIC vixen)

if(${CMAKE_SYSTEM_NAME} STREQUAL "Linux")
    target_link_libraries(vixen PRIVATE dl)
    target_compile_options(vixen PRIVATE -Wall -Wextra -Wpedantic -Wno-unknown-pragmas -Wno-unused-parameter -Werror)
    target_compile_options(vixentest PRIVATE -Wall -Wextra -Wpedantic -Wno-unknown-pragmas -Wno-unused-parameter -Werror)
endif()

# debug/profiling
option(VIXEN_ENABLE_TRACY "Enable Tracy Integration" OFF)

if(${VIXEN_ENABLE_TRACY})
    add_library(tracy STATIC
        "${CMAKE_CURRENT_SOURCE_DIR}/../lib/tracy/Tracy.hpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/../lib/tracy/TracyClient.cpp"
    )

    target_include_directories(tracy PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/../lib/tracy")
    target_compile_definitions(tracy PUBLIC TRACY_ENABLE)

    target_link_libraries(vixen PUBLIC tracy)
endif()

option(VIXEN_ENABLE_UBSAN "Enable Undefined Behavior Sanitizer" OFF)

if(${VIXEN_ENABLE_UBSAN})
    target_compile_options(vixen PRIVATE -fsanitize=undefined)
    target_link_options(vixen PUBLIC -fsanitize=undefined)
endif()

option(VIXEN_ENABLE_ASAN "Enable Address Sanitizer" OFF)

if(${VIXEN_ENABLE_ASAN})
    target_compile_options(vixen PRIVATE -fsanitize=address)
    target_link_options(vixen PUBLIC -fsanitize=address)
endif()

# platform definitions
set(WINDOWS_PLATFORMS "Windows" "CYGWIN_NT-5.1" "MSYS_NT-6.1")

if(${CMAKE_SYSTEM_NAME} STREQUAL "Linux")
    target_compile_definitions(vixen PUBLIC VIXEN_PLATFORM_LINUX)
elseif(${CMAKE_SYSTEM_NAME} IN_LIST WINDOWS_PLATFORMS)
    target_compile_definitions(vixen PUBLIC VIXEN_PLATFORM_WINDOWS)

    # i dont fucking know
    target_compile_definitions(vixen PUBLIC _AMD64)
    target_link_libraries(vixen PRIVATE -ldbghelp)

    set_target_properties(vixen PROPERTIES PDB_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}")
endif()

# documentation generation
option(VIXEN_BUILD_DOCS "Build documentation (uses Doxygen)" ON)

if(VIXEN_BUILD_DOCS)
    find_package(Doxygen)

    if(DOXYGEN_FOUND)
        set(DOXYFILE ${CMAKE_CURRENT_SOURCE_DIR}/docs/Doxyfile.in)
        set(CONFIGURED_DOXYFILE ${CMAKE_CURRENT_SOURCE_DIR}/docs/Doxyfile.out)

        configure_file(${DOXYFILE} ${CONFIGURED_DOXYFILE} @ONLY)
        message("Doxygen build started")

        add_custom_target(vixen_docs
            COMMAND ${DOXYGEN_EXECUTABLE} ${CONFIGURED_DOXYFILE}
            WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/docs
            VERBATIM
        )
    else()
        message(ERROR "Doxygen nout found. Documentation could not be built.")
    endif()
endif()
