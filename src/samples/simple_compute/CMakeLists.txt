set(RENDER_SOURCE
        simple_compute.cpp
        compute_init.cpp
        execute.cpp)

add_executable(simple_compute main.cpp ${VK_UTILS_SRC} ${RENDER_SOURCE})

if(CMAKE_SYSTEM_NAME STREQUAL Windows)
    set_target_properties(simple_compute PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}")

    target_link_libraries(simple_compute PRIVATE project_options
                          etna project_warnings)
else()
    target_link_libraries(simple_compute PRIVATE project_options
                          etna project_warnings) #
endif()