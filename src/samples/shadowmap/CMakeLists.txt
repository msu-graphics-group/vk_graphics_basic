set(RENDER_SOURCE
  ../../render/ImGuiRender.cpp
  ../../render/quad_renderer.cpp
  Renderer.cpp
  render_init.cpp
  update.cpp
  draw.cpp
  gui.cpp
  App.cpp
)

add_executable(shadowmap main.cpp ${VK_UTILS_SRC} ${SCENE_LOADER_SRC} ${RENDER_SOURCE})

target_link_libraries(shadowmap PRIVATE glfw etna glm::glm wsi DearImGui scene)
