cmake_minimum_required(VERSION 3.15)

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/bin)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/bin)

project(vk_graphics_basic CXX)
include(cmake/StandardProjectSettings.cmake)

add_library(project_options INTERFACE)
target_compile_features(project_options INTERFACE cxx_std_20)

# Link this 'library' to use the warnings specified in CompilerWarnings.cmake
add_library(project_warnings INTERFACE)

# standard compiler warnings
include(cmake/CompilerWarnings.cmake)
set_project_warnings(project_warnings)

add_compile_definitions(IMGUI_USER_CONFIG="${CMAKE_CURRENT_SOURCE_DIR}/src/render/my_imgui_config.h")
##############################################
# common sources used by all samples

# vk_utils source code files used by this project
# you can add other files (i.e. ray tracing helpers) when you'll need them
set(VK_UTILS_SRC
        ${CMAKE_SOURCE_DIR}/external/vkutils/vk_copy.cpp
        ${CMAKE_SOURCE_DIR}/external/vkutils/vk_utils.cpp
        ${CMAKE_SOURCE_DIR}/external/vkutils/vk_buffers.cpp
        ${CMAKE_SOURCE_DIR}/external/vkutils/vk_descriptor_sets.cpp
        ${CMAKE_SOURCE_DIR}/external/vkutils/vk_images.cpp
        ${CMAKE_SOURCE_DIR}/external/vkutils/vk_swapchain.cpp
        ${CMAKE_SOURCE_DIR}/external/vkutils/vk_pipeline.cpp
        ${CMAKE_SOURCE_DIR}/external/vkutils/geom/vk_mesh.cpp
        ${CMAKE_SOURCE_DIR}/external/vkutils/geom/cmesh.cpp
        ${CMAKE_SOURCE_DIR}/external/vkutils/vk_fbuf_attachment.cpp)

set(SCENE_LOADER_SRC
        ${CMAKE_SOURCE_DIR}/src/loader_utils/pugixml.cpp
        ${CMAKE_SOURCE_DIR}/src/loader_utils/hydraxml.cpp
        ${CMAKE_SOURCE_DIR}/src/loader_utils/images.cpp)

set(IMGUI_SRC
        ${CMAKE_SOURCE_DIR}/external/imgui/imgui.cpp
        ${CMAKE_SOURCE_DIR}/external/imgui/imgui_draw.cpp
        ${CMAKE_SOURCE_DIR}/external/imgui/imgui_tables.cpp
        ${CMAKE_SOURCE_DIR}/external/imgui/imgui_widgets.cpp
        ${CMAKE_SOURCE_DIR}/external/imgui/imgui_demo.cpp
        ${CMAKE_SOURCE_DIR}/external/imgui/backends/imgui_impl_vulkan.cpp
        ${CMAKE_SOURCE_DIR}/external/imgui/backends/imgui_impl_glfw.cpp
        ${CMAKE_SOURCE_DIR}/external/imgui/misc/cpp/imgui_stdlib.cpp)

include_directories(${CMAKE_SOURCE_DIR}/external/imgui)
include_directories(${CMAKE_SOURCE_DIR}/external/vkutils)
include_directories(${CMAKE_SOURCE_DIR}/external)
include_directories(${CMAKE_SOURCE_DIR}/src)
#include_directories(${CMAKE_SOURCE_DIR}/external/volk)
##############################################

add_compile_definitions(USE_ETNA VK_GRAPHICS_BASIC_ROOT="${PROJECT_SOURCE_DIR}")

add_subdirectory(external/etna)
#add_subdirectory(external/volk)
#add_subdirectory(src/samples/quad2d)
add_subdirectory(src/samples/shadowmap)
#add_subdirectory(src/samples/simpleforward)
add_subdirectory(src/samples/simple_compute)
