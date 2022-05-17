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

set(PUB_INCLUDE_PREFIX "${CMAKE_CURRENT_SOURCE_DIR}/interface/public/vixen/")
set(VIXEN_PUBLIC_HEADER_FILES
    "${PUB_INCLUDE_PREFIX}/allocator/allocator.hpp"
    "${PUB_INCLUDE_PREFIX}/allocator/allocators.hpp"
    "${PUB_INCLUDE_PREFIX}/allocator/layout.hpp"
    "${PUB_INCLUDE_PREFIX}/allocator/pool_allocator.hpp"
    "${PUB_INCLUDE_PREFIX}/allocator/profile.hpp"
    "${PUB_INCLUDE_PREFIX}/allocator/stacktrace.hpp"
    "${PUB_INCLUDE_PREFIX}/io/file_stream.hpp"
    "${PUB_INCLUDE_PREFIX}/io/file.hpp"
    "${PUB_INCLUDE_PREFIX}/any.hpp"
    "${PUB_INCLUDE_PREFIX}/assert.hpp"
    "${PUB_INCLUDE_PREFIX}/common.hpp"
    "${PUB_INCLUDE_PREFIX}/defer.hpp"
    "${PUB_INCLUDE_PREFIX}/defines.hpp"
    "${PUB_INCLUDE_PREFIX}/function.hpp"
    "${PUB_INCLUDE_PREFIX}/log.hpp"
    "${PUB_INCLUDE_PREFIX}/option.hpp"
    "${PUB_INCLUDE_PREFIX}/prof.hpp"
    "${PUB_INCLUDE_PREFIX}/result.hpp"
    "${PUB_INCLUDE_PREFIX}/shared.hpp"
    "${PUB_INCLUDE_PREFIX}/slice.hpp"
    "${PUB_INCLUDE_PREFIX}/string.hpp"
    "${PUB_INCLUDE_PREFIX}/traits.hpp"
    "${PUB_INCLUDE_PREFIX}/tuple.hpp"
    "${PUB_INCLUDE_PREFIX}/typeops.hpp"
    "${PUB_INCLUDE_PREFIX}/types.hpp"
    "${PUB_INCLUDE_PREFIX}/unique.hpp"
    "${PUB_INCLUDE_PREFIX}/util.hpp"
    "${PUB_INCLUDE_PREFIX}/vector.hpp"
)

set(PRIV_INCLUDE_PREFIX "${CMAKE_CURRENT_SOURCE_DIR}/interface/internal/vixen/")
set(VIXEN_PRIVATE_HEADER_FILES
    "${PRIV_INCLUDE_PREFIX}/hash/hasher.hpp"
    "${PRIV_INCLUDE_PREFIX}/hash/map.hpp"
    "${PRIV_INCLUDE_PREFIX}/hash/set.hpp"
    "${PRIV_INCLUDE_PREFIX}/hash/table.hpp"
)

set(LIB_NAME "vixen")
add_library(${LIB_NAME} STATIC
    ${VIXEN_SOURCE_FILES}
    ${VIXEN_PUBLIC_HEADER_FILES}
    ${VIXEN_PRIVATE_HEADER_FILES}
)

target_include_directories(${LIB_NAME} PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/interface/public")
target_include_directories(${LIB_NAME} PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/interface/internal")

# for .inl files
target_include_directories(${LIB_NAME} PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/src/vixen")

target_compile_features(${LIB_NAME} PRIVATE cxx_std_20)

target_link_libraries(${LIB_NAME} PUBLIC spdlog)

if(${CMAKE_SYSTEM_NAME} STREQUAL "Linux")
    target_link_libraries(${LIB_NAME} PRIVATE dl)
endif()

target_compile_options(${LIB_NAME} PRIVATE -Wall -Wextra -Wpedantic -Wno-unknown-pragmas -Wno-unused-parameter -Werror)
target_compile_options(${LIB_NAME} PRIVATE -fmax-errors=3)

# debug/profiling
option(VIXEN_ENABLE_TRACY "Enable Tracy Integration" OFF)

if(${VIXEN_ENABLE_TRACY})
    add_library(tracy STATIC
        "${CMAKE_CURRENT_SOURCE_DIR}/../lib/tracy/Tracy.hpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/../lib/tracy/TracyClient.cpp"
    )

    target_include_directories(tracy PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/../lib/tracy")
    target_compile_definitions(tracy PUBLIC TRACY_ENABLE)

    target_link_libraries(${LIB_NAME} PUBLIC tracy)
endif()

option(VIXEN_ENABLE_UBSAN "Enable Undefined Behavior Sanitizer" OFF)

if(${VIXEN_ENABLE_UBSAN})
    target_compile_options(${LIB_NAME} PRIVATE -fsanitize=undefined)
    target_link_options(${LIB_NAME} PUBLIC -fsanitize=undefined)
endif()

option(VIXEN_ENABLE_ASAN "Enable Address Sanitizer" OFF)

if(${VIXEN_ENABLE_ASAN})
    target_compile_options(${LIB_NAME} PRIVATE -fsanitize=address)
    target_link_options(${LIB_NAME} PUBLIC -fsanitize=address)
endif()

# platform definitions
set(WINDOWS_PLATFORMS "Windows" "CYGWIN_NT-5.1" "MSYS_NT-6.1")

if(${CMAKE_SYSTEM_NAME} STREQUAL "Linux")
    target_compile_definitions(${LIB_NAME} PUBLIC VIXEN_PLATFORM_LINUX)
elseif(${CMAKE_SYSTEM_NAME} IN_LIST WINDOWS_PLATFORMS)
    target_compile_definitions(${LIB_NAME} PUBLIC VIXEN_PLATFORM_WINDOWS)
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
