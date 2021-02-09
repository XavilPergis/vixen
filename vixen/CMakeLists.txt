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

file(GLOB_RECURSE VIXEN_SOURCE_FILES "${CMAKE_CURRENT_SOURCE_DIR}/impl/obj/*.cpp")
file(GLOB_RECURSE VIXEN_HEADER_FILES "${CMAKE_CURRENT_SOURCE_DIR}/interface/*.hpp")

add_library(vixen ${VIXEN_HEADER_FILES} ${VIXEN_SOURCE_FILES})
target_include_directories(vixen PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/interface/public")
target_include_directories(vixen PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/interface/internal")
target_include_directories(vixen PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/impl/inl")

target_compile_features(vixen PRIVATE cxx_std_17)

target_link_libraries(vixen PUBLIC spdlog)
target_link_libraries(vixen PRIVATE dl)

target_compile_options(vixen PUBLIC -fsanitize=undefined -fsanitize=address -fdiagnostics-color=always)
target_link_options(vixen PUBLIC -fsanitize=undefined -fsanitize=address)
