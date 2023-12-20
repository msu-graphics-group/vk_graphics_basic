#include "quad2d_render.h"

#include <etna/Etna.hpp>


SimpleQuad2dRender::SimpleQuad2dRender(uint32_t a_width, uint32_t a_height) : m_width(a_width), m_height(a_height)
{
}

void SimpleQuad2dRender::InitVulkan(const char** a_instanceExtensions, uint32_t a_instanceExtensionsCount, uint32_t)
{
  for(size_t i = 0; i < a_instanceExtensionsCount; ++i)
  {
    m_instanceExtensions.push_back(a_instanceExtensions[i]);
  }

  #ifndef NDEBUG
    m_instanceExtensions.push_back("VK_EXT_debug_report");
  #endif

  SetupDeviceExtensions();
  
  etna::initialize(etna::InitParams
    {
      .applicationName = "Quad2dSample",
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

  m_pCopyHelper = std::make_shared<vk_utils::SimpleCopyHelper>(m_context->getPhysicalDevice(), m_context->getDevice(), m_context->getQueue(), m_context->getQueueFamilyIdx(), 8 * 1024 * 1024);
}

void SimpleQuad2dRender::SetupDeviceExtensions()
{
  m_deviceExtensions.push_back(VK_KHR_SWAPCHAIN_EXTENSION_NAME);
}

void SimpleQuad2dRender::RecreateSwapChain()
{
  ETNA_ASSERT(m_context->getDevice().waitIdle() == vk::Result::eSuccess);

  DeallocateResources();

  DestroyPipelines();
  ResetPresentStuff();

  auto oldImgNum = m_swapchain.GetImageCount();
  m_presentationResources.queue = m_swapchain.CreateSwapChain(
    m_context->getPhysicalDevice(), m_context->getDevice(), m_surface, m_width, m_height,
         oldImgNum, m_vsync);

  InitPresentStuff();

  PreparePipelines();
}

SimpleQuad2dRender::~SimpleQuad2dRender()
{
  DeallocateResources();
  DestroyPipelines();
  ResetPresentStuff();
}