#include "shadowmap_render.h"

#include "../../render/ImGuiRender.h"
#include "vk_utils.h"


void SimpleShadowmapRender::InitPresentation(vk::UniqueSurfaceKHR a_surface, etna::ResolutionProvider a_res_provider)
{
  {
    m_commandCtrl.emplace();

    const auto &workCount = m_context->getMainWorkCount();

    vk::CommandPool commandPool = vk_utils::createCommandPool(m_context->getDevice(), m_context->getQueueFamilyIdx(), VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);
    m_commandCtrl->commandPool  = vk::UniqueCommandPool{
      commandPool,
      vk::ObjectDestroy<vk::Device, VULKAN_HPP_DEFAULT_DISPATCHER_TYPE>{ m_context->getDevice() }
    };

    auto bufs = vk_utils::createCommandBuffers(m_context->getDevice(), m_commandCtrl->commandPool.get(), workCount.multiBufferingCount());
    m_commandCtrl->cmdBuffersDrawMain.emplace(workCount, [&](std::size_t i) {
      return vk::UniqueCommandBuffer{
        bufs[i],
        vk::PoolFree<vk::Device, vk::CommandPool, VULKAN_HPP_DEFAULT_DISPATCHER_TYPE>{ m_context->getDevice(), commandPool }
      };
    });
  }

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

    auto [w, h] = m_frameCtrl->window->recreateSwapchain();
    m_width     = w;
    m_height    = h;
  }

  m_pGUIRender = std::make_unique<ImGuiRender>(m_frameCtrl->window->getCurrentFormat());

  AllocateResources();
}
