#include "shadowmap_render.h"
#include <utils/glfw_window.h>
#include <etna/Etna.hpp>
#include <backends/imgui_impl_glfw.h>
#include <GLFW/glfw3.h>


void initVulkanGLFW(SimpleShadowmapRender &app, GLFWwindow* window)
{
  uint32_t glfwExtensionCount = 0;
  const char** glfwExtensions;
  glfwExtensions  = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

  if(glfwExtensions == nullptr)
  {
    ETNA_PANIC("Can't connect Vulkan to GLFW window (glfwGetRequiredInstanceExtensions returned null)");
  }

  app.InitVulkan(glfwExtensions, glfwExtensionCount);

  VkSurfaceKHR surface;
  auto instance = etna::get_context().getInstance();
  auto cres = glfwCreateWindowSurface(instance, window, nullptr, &surface);
  ETNA_CHECK_VK_RESULT(static_cast<vk::Result>(cres));
  setupImGuiContext(window);
  app.InitPresentation(vk::UniqueSurfaceKHR{std::move(surface)}, [window]() -> vk::Extent2D {
      int w, h;
      glfwGetWindowSize(window, &w, &h);
      ETNA_ASSERT(w > 0 && h > 0);
      return {static_cast<uint32_t>(w), static_cast<uint32_t>(h)};
  });
}

int main()
{
  constexpr int WIDTH = 1024;
  constexpr int HEIGHT = 1024;

  {
    SimpleShadowmapRender renderer(WIDTH, HEIGHT);

    auto* window = initWindow(WIDTH, HEIGHT);

    initVulkanGLFW(renderer, window);

    renderer.LoadScene(VK_GRAPHICS_BASIC_ROOT "/resources/scenes/043_cornell_normals/statex_00001.xml", false);

    mainLoop(renderer, window, true);
  }

  if (etna::is_initilized())
    etna::shutdown();

  return 0;
}
