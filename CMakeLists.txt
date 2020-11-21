cmake_minimum_required(VERSION 3.15.2)
project(Xen)

add_subdirectory(lib/spdlog)

add_subdirectory(xen_core)
add_subdirectory(xen_data)
