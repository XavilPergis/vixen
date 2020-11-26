cmake_minimum_required(VERSION 3.15.2)

file(GLOB_RECURSE VIXEN_SOURCE_FILES "${CMAKE_CURRENT_SOURCE_DIR}/impl/obj/*.cpp")
file(GLOB_RECURSE VIXEN_HEADER_FILES "${CMAKE_CURRENT_SOURCE_DIR}/interface/*.hpp")

add_library(vixen ${VIXEN_HEADER_FILES} ${VIXEN_SOURCE_FILES})
target_include_directories(vixen PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/interface")
target_include_directories(vixen PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/impl/inl")

target_compile_features(vixen PRIVATE cxx_std_17)

target_link_libraries(vixen PUBLIC spdlog)
target_link_libraries(vixen PRIVATE dl)
