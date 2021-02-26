cmake_minimum_required(VERSION 3.15.2)

file(GLOB_RECURSE VIXEN_EXAMPLE_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp")

option(VIXEN_BUILD_EXAMPLES "Build Vixen Examples" OFF)

if (${VIXEN_BUILD_EXAMPLES})
    foreach(example_source ${VIXEN_EXAMPLE_SOURCES})
        string(REPLACE "${CMAKE_CURRENT_SOURCE_DIR}/" "" example_file ${example_source})
        string(REPLACE ".cpp" "" example_name ${example_file})
        
        add_executable("vixen-example-${example_name}" ${example_source})
        target_link_libraries("vixen-example-${example_name}" PRIVATE vixen)
        target_compile_features("vixen-example-${example_name}" PUBLIC cxx_std_17)
    endforeach(example_source ${VIXEN_EXAMPLE_SOURCES})
endif()