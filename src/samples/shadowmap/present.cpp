#include "shadowmap_render.h"

#include "../../render/ImGuiRender.h"
#include "vk_utils.h"


void SimpleShadowmapRender::InitPresentStuff()
{
  // TODO: Move to customizable initialization

  m_commandCtrl.emplace();

  const auto &workCount = m_context->getMainWorkCount();

  m_commandCtrl->commandPool = vk::UniqueCommandPool{ vk_utils::createCommandPool(m_context->getDevice(), m_context->getQueueFamilyIdx(), VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT) };
  auto bufs                  = vk_utils::createCommandBuffers(m_context->getDevice(), m_commandCtrl->commandPool.get(), workCount.multiBufferingCount());
  m_commandCtrl->cmdBuffersDrawMain.emplace(workCount, [&bufs](std::size_t i) {
    return vk::UniqueCommandBuffer{ std::move(bufs[i]) };
  });

  auto[w, h] = m_frameCtrl->window->recreateSwapchain();
  m_width = w;
  m_height = h;

  m_pGUIRender = std::make_unique<ImGuiRender>(m_frameCtrl->window->getCurrentFormat());
}

void SimpleShadowmapRender::ResetPresentStuff()
{
  m_commandCtrl.reset();
  m_frameCtrl.reset();
}

void SimpleShadowmapRender::InitPresentation(vk::UniqueSurfaceKHR a_surface, etna::ResolutionProvider a_res_provider)
{
  m_frameCtrl.emplace();

  m_frameCtrl->renderingFinished = etna::unwrap_vk_result(m_context->getDevice().createSemaphoreUnique(vk::SemaphoreCreateInfo{}));
  m_frameCtrl->frameDone.emplace(m_context->getMainWorkCount(), [this](std::size_t) {
    vk::FenceCreateInfo info{
      .flags = vk::FenceCreateFlagBits::eSignaled
    };
    return etna::unwrap_vk_result(m_context->getDevice().createFenceUnique(info));
  });

  m_frameCtrl->window = m_context->createWindow(etna::Window::CreateInfo{
    .surface            = std::move(a_surface),
    .resolutionProvider = std::move(a_res_provider),
  });

  InitPresentStuff();
  AllocateResources();
}
