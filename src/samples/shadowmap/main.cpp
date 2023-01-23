#include "shadowmap_render.h"
#include "utils/glfw_window.h"
#include <etna/Etna.hpp>

void initVulkanGLFW(std::shared_ptr<IRender> &app, GLFWwindow* window)
{
  uint32_t glfwExtensionCount = 0;
  const char** glfwExtensions;
  glfwExtensions  = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

  if(glfwExtensions == nullptr)
  {
    std::cout << "WARNING. Can't connect Vulkan to GLFW window (glfwGetRequiredInstanceExtensions returns NULL)" << std::endl;
  }

  app->InitVulkan(glfwExtensions, glfwExtensionCount, /* useless param */ 0);

  if(glfwExtensions != nullptr)
  {
    VkSurfaceKHR surface;
    VK_CHECK_RESULT(glfwCreateWindowSurface(app->GetVkInstance(), window, nullptr, &surface));
    setupImGuiContext(window);
    app->InitPresentation(surface, false);
  }
}

int main()
{
  constexpr int WIDTH = 1024;
  constexpr int HEIGHT = 1024;

  std::shared_ptr<IRender> app = std::make_unique<SimpleShadowmapRender>(WIDTH, HEIGHT);
  if(app == nullptr)
  {
    std::cout << "Can't create render of specified type" << std::endl;
    return 1;
  }

  auto* window = initWindow(WIDTH, HEIGHT);

  initVulkanGLFW(app, window);

  app->LoadScene(VK_GRAPHICS_BASIC_ROOT "/resources/scenes/043_cornell_normals/statex_00001.xml", false);

  mainLoop(app, window, true);

  app = {};
  if (etna::is_initilized())
    etna::shutdown();

  return 0;
}
