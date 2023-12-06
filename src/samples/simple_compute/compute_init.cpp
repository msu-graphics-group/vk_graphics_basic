#include "simple_compute.h"

#include <etna/Etna.hpp>


SimpleCompute::SimpleCompute(uint32_t a_length) : m_length(a_length) {}

void SimpleCompute::InitVulkan(const char** a_instanceExtensions, uint32_t a_instanceExtensionsCount, uint32_t a_deviceId)
{
  for (uint32_t i = 0; i < a_instanceExtensionsCount; ++i) {
    m_instanceExtensions.push_back(a_instanceExtensions[i]);
  }

  #ifndef NDEBUG
    m_instanceExtensions.push_back("VK_EXT_debug_report");
  #endif
  
  etna::initialize(etna::InitParams
    {
      .applicationName = "ComputeSample",
      .applicationVersion = VK_MAKE_VERSION(0, 1, 0),
      .instanceExtensions = m_instanceExtensions,
      .deviceExtensions = m_deviceExtensions,
      // .physicalDeviceIndexOverride = 0,
    }
  );

  m_context = &etna::get_context();
  m_commandPool = vk_utils::createCommandPool(m_context->getDevice(), m_context->getQueueFamilyIdx(), VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);
  
  m_cmdBufferCompute = vk_utils::createCommandBuffers(m_context->getDevice(), m_commandPool, 1)[0];
  
  m_pCopyHelper = std::make_shared<vk_utils::SimpleCopyHelper>(m_context->getPhysicalDevice(), 
    m_context->getDevice(), m_context->getQueue(), m_context->getQueueFamilyIdx(), 8 * 1024 * 1024);
}

void SimpleCompute::Cleanup()
{
  CleanupPipeline();

  if (m_commandPool != VK_NULL_HANDLE)
  {
    vkDestroyCommandPool(m_context->getDevice(), m_commandPool, nullptr);
  }
}
