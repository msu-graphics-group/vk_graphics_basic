#include "simple_compute.h"

#include <etna/Etna.hpp>


SimpleCompute::SimpleCompute(uint32_t a_length) : m_length(a_length) {}

void SimpleCompute::Init()
{
  std::vector<const char*> instanceExtensions = {};

  #ifndef NDEBUG
    instanceExtensions.push_back("VK_EXT_debug_report");
  #endif

  etna::initialize(etna::InitParams
    {
      .applicationName = "ComputeSample",
      .applicationVersion = VK_MAKE_VERSION(0, 1, 0),
      .instanceExtensions = instanceExtensions,
      .deviceExtensions = {},
      // Uncomment if etna selects the incorrect GPU for you
      // .physicalDeviceIndexOverride = 0,
    }
  );

  m_context = &etna::get_context();


  {
    vk::CommandPoolCreateInfo info{
      .flags = vk::CommandPoolCreateFlagBits::eResetCommandBuffer,
      .queueFamilyIndex = m_context->getQueueFamilyIdx(),
    };
    m_commandPool = etna::unwrap_vk_result(m_context->getDevice().createCommandPoolUnique(info));
  }

  {
    vk::CommandBufferAllocateInfo info{
      .commandPool = m_commandPool.get(),
      .level = vk::CommandBufferLevel::ePrimary,
      .commandBufferCount = 1,
    };
    m_cmdBufferCompute = std::move(etna::unwrap_vk_result(m_context->getDevice().allocateCommandBuffersUnique(info))[0]);
  }

  m_pCopyHelper = std::make_unique<vk_utils::SimpleCopyHelper>(m_context->getPhysicalDevice(),
    m_context->getDevice(), m_context->getQueue(), m_context->getQueueFamilyIdx(), 8 * 1024 * 1024);

  m_fence = etna::unwrap_vk_result(m_context->getDevice().createFenceUnique(vk::FenceCreateInfo{}));
}
