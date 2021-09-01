include_directories(../external/volk)
include_directories(../external/vkutils)
include_directories(../external)

if(CMAKE_SYSTEM_NAME STREQUAL Windows)
    include_directories(../external/glfw/include)
    link_directories(../external/glfw)
else()
    find_package(glfw3 REQUIRED)
    include_directories(${GLFW_INCLUDE_DIRS})
endif()

set(SOURCE_FILES
        ../external/vkutils/vk_copy.cpp
        ../external/vkutils/vk_utils.cpp
        ../external/vkutils/vk_buffers.cpp
        ../external/vkutils/vk_descriptor_sets.cpp
        ../external/vkutils/vk_images.cpp
        ../external/vkutils/vk_swapchain.cpp
        ../external/vkutils/vk_pipeline.cpp
        ../external/vkutils/geom/vk_mesh.cpp
        ../external/vkutils/geom/cmesh.cpp
        ../external/vkutils/vk_quad.cpp
        ../external/vkutils/vk_fbuf_attachment.cpp

        loader_utils/pugixml.cpp
        loader_utils/hydraxml.cpp

        render/create_render.cpp
        render/scene_mgr.cpp
        render/simple_render.cpp
        
        render/shadowmap_render.cpp)

add_executable(renderer main.cpp glfw_window.cpp ${SOURCE_FILES}) #

if(CMAKE_SYSTEM_NAME STREQUAL Windows)
    set_target_properties(renderer PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}")

    target_link_libraries(renderer PRIVATE project_options
                          volk glfw3 project_warnings)
else()
    target_link_libraries(renderer PRIVATE project_options
                          volk glfw project_warnings) #
endif()