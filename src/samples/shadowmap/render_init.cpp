#include "shadowmap_render.h"

#include <etna/Etna.hpp>

#include "render/ImGuiRender.h"


SimpleShadowmapRender::SimpleShadowmapRender(uint32_t a_width, uint32_t a_height) : m_width(a_width), m_height(a_height)
{
  m_uniforms.baseColor = LiteMath::float3(0.9f, 0.92f, 1.0f);
}

void SimpleShadowmapRender::InitVulkan(const char** a_instanceExtensions, uint32_t a_instanceExtensionsCount)
{
  std::vector<const char*> m_instanceExtensions;

  for(size_t i = 0; i < a_instanceExtensionsCount; ++i)
  {
    m_instanceExtensions.push_back(a_instanceExtensions[i]);
  }

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
      .physicalDeviceIndexOverride = {}
    });

  m_context = &etna::get_context();

  m_pScnMgr = std::make_shared<SceneManager>(
    m_context->getDevice(), m_context->getPhysicalDevice(),
    m_context->getQueueFamilyIdx(), m_context->getQueueFamilyIdx(), false);
}

void SimpleShadowmapRender::RecreateSwapChain()
{
  // TODO: this doesn't work 100%
  ETNA_CHECK_VK_RESULT(m_context->getDevice().waitIdle());

  auto[w, h] = m_frameCtrl->window->recreateSwapchain();
  m_width = w;
  m_height = h;

  AllocateResources();

  // NOTE: if swapchain changes format (that can happen on android), we will die here.
  // not that we actually care about android or anything.
}

SimpleShadowmapRender::~SimpleShadowmapRender()
{
  ETNA_CHECK_VK_RESULT(m_context->getDevice().waitIdle());

}