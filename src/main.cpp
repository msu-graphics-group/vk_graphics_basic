#include "render/render_common.h"
#include "glfw_window.h"

void initVulkanGLFW(std::shared_ptr<IRender> &app, GLFWwindow* window, int deviceID)
{
  uint32_t glfwExtensionCount = 0;
  const char** glfwExtensions;
  glfwExtensions  = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

  if(glfwExtensions == nullptr)
  {
    std::cout << "WARNING. Can't connect Vulkan to GLFW window (glfwGetRequiredInstanceExtensions returns NULL)" << std::endl;
  }

  app->InitVulkan(glfwExtensions, glfwExtensionCount, deviceID);

  if(glfwExtensions != nullptr)
  {
    VkSurfaceKHR surface;
    VK_CHECK_RESULT(glfwCreateWindowSurface(app->GetVkInstance(), window, nullptr, &surface));
    setupImGuiContext(window);
    app->InitPresentation(surface);
  }
}

int main()
{
  constexpr int WIDTH = 2048;
  constexpr int HEIGHT = 2048;
  constexpr int VULKAN_DEVICE_ID = 0;

  std::shared_ptr<IRender> app = CreateRender(WIDTH, HEIGHT, RenderEngineType::SHADOW_MAP);
  if(app == nullptr)
  {
    std::cout << "Can't create render of specified type" << std::endl;
    return 1;
  }

  auto* window = initWindow(WIDTH, HEIGHT);

  initVulkanGLFW(app, window, VULKAN_DEVICE_ID);

  app->LoadScene("../resources/scenes/043_cornell_normals/statex_00001.xml", false);

  bool showGUI = true;
  mainLoop(app, window, showGUI);

  return 0;
}
