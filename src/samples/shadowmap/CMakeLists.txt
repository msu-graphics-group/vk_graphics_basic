if(CMAKE_SYSTEM_NAME STREQUAL Windows)
    include_directories(${CMAKE_SOURCE_DIR}/external/glfw/include)
    link_directories(${CMAKE_SOURCE_DIR}/external/glfw)
else()
    find_package(glfw3 REQUIRED)
    include_directories(${GLFW_INCLUDE_DIRS})
endif()


set(RENDER_SOURCE
        ../../render/scene_mgr.cpp
        ../../render/render_imgui.cpp
        ../../render/quad_renderer.cpp
        shadowmap_render.cpp
        render_init.cpp
        update.cpp
        draw.cpp
        present.cpp
        gui.cpp
)

add_executable(shadowmap_renderer main.cpp ../../utils/glfw_window.cpp ${VK_UTILS_SRC} ${SCENE_LOADER_SRC} ${RENDER_SOURCE} ${IMGUI_SRC})

if(CMAKE_SYSTEM_NAME STREQUAL Windows)
    set_target_properties(shadowmap_renderer PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}")

    target_link_libraries(shadowmap_renderer PRIVATE project_options
                          glfw3 project_warnings etna ${CMAKE_DL_LIBS})
else()
    target_link_libraries(shadowmap_renderer PRIVATE project_options
                          glfw project_warnings etna ${CMAKE_DL_LIBS}) #
endif()