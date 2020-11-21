cmake_minimum_required(VERSION 3.15.2)

set(XEN_BASE_PATH "${CMAKE_SOURCE_DIR}")

file(GLOB_RECURSE XEN_SOURCE_FILES "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp")
file(GLOB_RECURSE XEN_HEADER_FILES "${CMAKE_CURRENT_SOURCE_DIR}/include/*.hpp")

add_library(XenEngineCore ${XEN_HEADER_FILES} ${XEN_SOURCE_FILES})
target_include_directories(XenEngineCore PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")

target_compile_features(XenEngineCore PUBLIC cxx_std_17)

# spdlog
target_link_libraries(XenEngineCore PUBLIC spdlog)
