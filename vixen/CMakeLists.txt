cmake_minimum_required(VERSION 3.15.2)

option(VIXEN_BUILD_DOCS "Build documentation (uses Doxygen)" ON)

if (VIXEN_BUILD_DOCS)
    find_package(Doxygen)
    if (DOXYGEN_FOUND)
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


option(ENABLE_TRACY "Enable Tracy Integration" OFF)
add_library(tracy STATIC "${CMAKE_CURRENT_SOURCE_DIR}/../lib/tracy/Tracy.hpp" "${CMAKE_CURRENT_SOURCE_DIR}/../lib/tracy/TracyClient.cpp")
target_include_directories(tracy PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/../lib/tracy")

file(GLOB_RECURSE VIXEN_SOURCE_FILES "${CMAKE_CURRENT_SOURCE_DIR}/impl/obj/*.cpp")
file(GLOB_RECURSE VIXEN_HEADER_FILES "${CMAKE_CURRENT_SOURCE_DIR}/interface/*.hpp")
add_library(vixen STATIC ${VIXEN_HEADER_FILES} ${VIXEN_SOURCE_FILES})

if(${ENABLE_TRACY})
    target_compile_definitions(vixen PUBLIC TRACY_ENABLE)
    target_compile_definitions(tracy PUBLIC TRACY_ENABLE)
endif()

target_include_directories(vixen PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/interface/public")
target_include_directories(vixen PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/interface/internal")
target_include_directories(vixen PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/impl/inl")

target_compile_features(vixen PRIVATE cxx_std_17)

target_link_libraries(vixen PUBLIC tracy)
target_link_libraries(vixen PUBLIC spdlog)
target_link_libraries(vixen PRIVATE dl)

target_compile_options(vixen PRIVATE -fdiagnostics-color=always -Wall -Wextra -Wpedantic -Wno-unknown-pragmas -Wno-unused-parameter -Werror -fmax-errors=3)

option(VIXEN_ENABLE_UBSAN "Enable Undefined Behavior Sanitizer" OFF)
option(VIXEN_ENABLE_ASAN "Enable Address Sanitizer" OFF)

target_compile_options(vixen PRIVATE -fdiagnostics-color=always -fmax-errors=3)

if(${ENABLE_UBSAN})
    target_compile_options(vixen PRIVATE -fsanitize=undefined)
    target_link_options(vixen PUBLIC -fsanitize=undefined)
endif()
    
if(${ENABLE_ASAN})
    target_compile_options(vixen PRIVATE -fsanitize=address)
    target_link_options(vixen PUBLIC -fsanitize=address)
endif()

