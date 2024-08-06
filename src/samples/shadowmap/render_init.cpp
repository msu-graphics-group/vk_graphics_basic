#include "Renderer.h"

#include <etna/Etna.hpp>

#include "render/ImGuiRender.h"


Renderer::Renderer(glm::uvec2 res) : resolution{res}
{
  m_uniforms.baseColor = {0.9f, 0.92f, 1.0f};
}

void Renderer::initVulkan(std::span<const char*> instance_extensions)
{
  std::vector<const char*> m_instanceExtensions;

  for (auto ext : instance_extensions)
    m_instanceExtensions.push_back(ext);

  #ifndef NDEBUG
    m_instanceExtensions.push_back("VK_EXT_debug_report");
  #endif

  std::vector<const char*> m_deviceExtensions;

  m_deviceExtensions.push_back(VK_KHR_SWAPCHAIN_EXTENSION_NAME);

  etna::initialize(etna::InitParams
    {
      .applicationName = "ShadowmapSample",
      .applicationVersion = VK_MAKE_VERSION(0, 1, 0),
      .instanceExtensions = m_instanceExtensions,
      .deviceExtensions = m_deviceExtensions,
      .features = vk::PhysicalDeviceFeatures2
        {
          .features = m_enabledDeviceFeatures
        },
      // Replace with an index if etna detects your preferred GPU incorrectly
      .physicalDeviceIndexOverride = {},
      // How much frames we buffer on the GPU without waiting for their completion on the CPU
      .numFramesInFlight = 2
    });

  m_context = &etna::get_context();

  m_pScnMgr = std::make_shared<SceneManager>();
}

void Renderer::initPresentation(vk::UniqueSurfaceKHR a_surface, etna::ResolutionProvider a_res_provider)
{
  commandManager = m_context->createPerFrameCmdMgr();

  window = m_context->createWindow(etna::Window::CreateInfo{
    .surface            = std::move(a_surface),
    .resolutionProvider = std::move(a_res_provider),
  });

  if (auto maybeResolution = window->recreateSwapchain())
  {
    auto [w, h] = *maybeResolution;
    resolution = {w, h};
  }

  m_pGUIRender = std::make_unique<ImGuiRender>(window->getCurrentFormat());

  allocateResources();
}

void Renderer::recreateSwapchain()
{
  ETNA_CHECK_VK_RESULT(m_context->getDevice().waitIdle());

  // If we are minimized, we will simply spin and wait for un-minimization
  if (auto maybeRes = window->recreateSwapchain())
  {
    auto[w, h] = *maybeRes;
    resolution = {w, h};
  }

  // Most resources depend on the current resolution, so we recreate them.
  allocateResources();

  // NOTE: if swapchain changes format (that can happen on android), we will die here.
  // not that we actually care about android or anything.
}

Renderer::~Renderer()
{
  ETNA_CHECK_VK_RESULT(m_context->getDevice().waitIdle());
}
