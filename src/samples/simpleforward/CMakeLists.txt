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
        create_render.cpp
        simple_render.cpp
        simple_render_tex.cpp)

add_executable(simple_forward main.cpp ../../utils/glfw_window.cpp ${VK_UTILS_SRC} ${SCENE_LOADER_SRC} ${RENDER_SOURCE} ${IMGUI_SRC})

if(CMAKE_SYSTEM_NAME STREQUAL Windows)
    set_target_properties(simple_forward PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}")

    target_link_libraries(simple_forward PRIVATE project_options
                          volk glfw3 project_warnings)
else()
    target_link_libraries(simple_forward PRIVATE project_options
                          volk glfw project_warnings) #
endif()