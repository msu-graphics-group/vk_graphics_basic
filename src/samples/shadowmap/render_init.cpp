#include "shadowmap_render.h"

#include <etna/Etna.hpp>


SimpleShadowmapRender::SimpleShadowmapRender(uint32_t a_width, uint32_t a_height) : m_width(a_width), m_height(a_height)
{
}

void SimpleShadowmapRender::InitVulkan(const char** a_instanceExtensions, uint32_t a_instanceExtensionsCount, uint32_t a_deviceId)
{
  for(size_t i = 0; i < a_instanceExtensionsCount; ++i)
  {
    m_instanceExtensions.push_back(a_instanceExtensions[i]);
  }

  SetupDeviceExtensions();
  
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
    });
  
  m_context = &etna::get_context();

  m_pScnMgr = std::make_shared<SceneManager>(
    m_context->getDevice(), m_context->getPhysicalDevice(),
    m_context->getQueueFamilyIdx(), m_context->getQueueFamilyIdx(), false);
}

void SimpleShadowmapRender::SetupDeviceExtensions()
{
  m_deviceExtensions.push_back(VK_KHR_SWAPCHAIN_EXTENSION_NAME);
  m_deviceExtensions.push_back(VK_KHR_DYNAMIC_RENDERING_EXTENSION_NAME);
}

void SimpleShadowmapRender::RecreateSwapChain()
{
  m_context->getDevice().waitIdle();

  DeallocateResources();

  DestroyPipelines();
  ResetPresentStuff();

  auto oldImgNum = m_swapchain.GetImageCount();
  m_presentationResources.queue = m_swapchain.CreateSwapChain(
    m_context->getPhysicalDevice(), m_context->getDevice(), m_surface, m_width, m_height,
         oldImgNum, m_vsync);

  InitPresentStuff();

  AllocateResources();

  PreparePipelines();
}
