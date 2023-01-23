#include "shadowmap_render.h"

#include "../../render/render_gui.h"

void SimpleShadowmapRender::InitPresentStuff()
{
  VkSemaphoreCreateInfo semaphoreInfo = {};
  semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
  VK_CHECK_RESULT(vkCreateSemaphore(m_context->getDevice(), &semaphoreInfo, nullptr, &m_presentationResources.imageAvailable));
  VK_CHECK_RESULT(vkCreateSemaphore(m_context->getDevice(), &semaphoreInfo, nullptr, &m_presentationResources.renderingFinished));

  // TODO: Move to customizable initialization
  m_commandPool = vk_utils::createCommandPool(m_context->getDevice(), m_context->getQueueFamilyIdx(), VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);

  m_cmdBuffersDrawMain.reserve(m_framesInFlight);
  m_cmdBuffersDrawMain = vk_utils::createCommandBuffers(m_context->getDevice(), m_commandPool, m_framesInFlight);

  m_frameFences.resize(m_framesInFlight);
  VkFenceCreateInfo fenceInfo = {};
  fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
  fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;
  for (size_t i = 0; i < m_framesInFlight; i++)
  {
    VK_CHECK_RESULT(vkCreateFence(m_context->getDevice(), &fenceInfo, nullptr, &m_frameFences[i]));
  }

  m_pGUIRender = std::make_shared<ImGuiRender>(
    m_context->getInstance(),
    m_context->getDevice(),
    m_context->getPhysicalDevice(),
    m_context->getQueueFamilyIdx(),
    m_context->getQueue(),
    m_swapchain);
}

void SimpleShadowmapRender::ResetPresentStuff()
{
  if (!m_cmdBuffersDrawMain.empty())
  {
    vkFreeCommandBuffers(m_context->getDevice(), m_commandPool, static_cast<uint32_t>(m_cmdBuffersDrawMain.size()),
                         m_cmdBuffersDrawMain.data());
    m_cmdBuffersDrawMain.clear();
  }

  for (size_t i = 0; i < m_frameFences.size(); i++)
  {
    vkDestroyFence(m_context->getDevice(), m_frameFences[i], nullptr);
  }

  if (m_presentationResources.imageAvailable != VK_NULL_HANDLE)
  {
    vkDestroySemaphore(m_context->getDevice(), m_presentationResources.imageAvailable, nullptr);
  }
  if (m_presentationResources.renderingFinished != VK_NULL_HANDLE)
  {
    vkDestroySemaphore(m_context->getDevice(), m_presentationResources.renderingFinished, nullptr);
  }

  if (m_commandPool != VK_NULL_HANDLE)
  {
    vkDestroyCommandPool(m_context->getDevice(), m_commandPool, nullptr);
  }
}

void SimpleShadowmapRender::InitPresentation(VkSurfaceKHR &a_surface, bool)
{
  m_surface = a_surface;

  m_presentationResources.queue = m_swapchain.CreateSwapChain(
    m_context->getPhysicalDevice(), m_context->getDevice(), m_surface,
    m_width, m_height, m_framesInFlight, m_vsync);
  m_presentationResources.currentFrame = 0;

  AllocateResources();
  InitPresentStuff();
}
