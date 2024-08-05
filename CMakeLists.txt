cmake_minimum_required(VERSION 3.20)

project(vk_graphics_basic CXX)

# Prevent the user from polluting the root directory with build files
if(PROJECT_SOURCE_DIR STREQUAL PROJECT_BINARY_DIR)
  message(
    FATAL_ERROR
      "In-source builds not allowed. Please make a new directory (called a build directory) and run CMake from there."
    )
endif()


set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)

if (MSVC)
  add_compile_options(/W3)
else()
  add_compile_options(-Wall -Wextra)
endif()


include("get_cpm.cmake")
include("thirdparty.cmake")

# TODO: remove!! Legacy!!
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


include_directories(${CMAKE_SOURCE_DIR}/external)
include_directories(${CMAKE_SOURCE_DIR}/external/vkutils)


add_compile_definitions(USE_ETNA VK_GRAPHICS_BASIC_ROOT="${PROJECT_SOURCE_DIR}")

add_subdirectory(../etna etna)
add_subdirectory(src/samples/shadowmap)
add_subdirectory(src/samples/simple_compute)
add_subdirectory(src/wsi)
