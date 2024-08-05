set(RENDER_SOURCE
  ../../render/scene_mgr.cpp
  ../../render/ImGuiRender.cpp
  ../../render/quad_renderer.cpp
  shadowmap_render.cpp
  render_init.cpp
  update.cpp
  draw.cpp
  present.cpp
  gui.cpp
  App.cpp
)

add_executable(shadowmap_renderer main.cpp ${VK_UTILS_SRC} ${SCENE_LOADER_SRC} ${RENDER_SOURCE})

target_link_libraries(shadowmap_renderer PRIVATE glfw etna glm::glm wsi DearImGui)
