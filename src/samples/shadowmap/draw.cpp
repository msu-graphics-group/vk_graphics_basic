#include "shadowmap_render.h"
#include <etna/Etna.hpp>

#include <imgui/imgui.h>
#include "../../render/ImGuiRender.h"


void SimpleShadowmapRender::DrawFrameSimple(bool draw_gui)
{
  // We explicitly synchronize with previous usage of our
  // multiple-buffered resources by the GPU.
  std::array waitForInflightFrames{m_frameCtrl->frameDone->get().get()};
  ETNA_CHECK_VK_RESULT(m_context->getDevice().waitForFences(waitForInflightFrames, vk::True, UINT64_MAX));
  m_context->getDevice().resetFences(waitForInflightFrames);

  etna::begin_frame();

  auto nextSwapchainImage = m_frameCtrl->window->acquireNext();

  if (nextSwapchainImage)
  {
    auto[image, view, availableSem] = *nextSwapchainImage;
    auto currentCmdBuf = m_commandCtrl->cmdBuffersDrawMain->get().get();

    currentCmdBuf.reset({});

    ETNA_CHECK_VK_RESULT(currentCmdBuf.begin({ .flags = vk::CommandBufferUsageFlagBits::eSimultaneousUse }));
    BuildCommandBufferSimple(currentCmdBuf, image, view);

    if (draw_gui)
    {
      ImDrawData* pDrawData = ImGui::GetDrawData();
      m_pGUIRender->Draw(currentCmdBuf, {{0, 0}, {m_width, m_height}}, image, view, pDrawData);
    }

    etna::set_state(currentCmdBuf, image, vk::PipelineStageFlagBits2::eBottomOfPipe, {},
      vk::ImageLayout::ePresentSrcKHR, vk::ImageAspectFlagBits::eColor);

    etna::flush_barriers(currentCmdBuf);

    ETNA_CHECK_VK_RESULT(currentCmdBuf.end());

    std::array submitCmdBufs{currentCmdBuf};
    std::array waitSemos{availableSem};
    std::array waitStages{vk::PipelineStageFlags{vk::PipelineStageFlagBits::eColorAttachmentOutput}};
    std::array signalSemos{m_frameCtrl->renderingFinished.get()};

    vk::SubmitInfo submitInfo{
      .waitSemaphoreCount = waitSemos.size(),
      .pWaitSemaphores = waitSemos.data(),
      .pWaitDstStageMask = waitStages.data(),
      .commandBufferCount = submitCmdBufs.size(),
      .pCommandBuffers = submitCmdBufs.data(),
      .signalSemaphoreCount = signalSemos.size(),
      .pSignalSemaphores = signalSemos.data(),
    };

    ETNA_CHECK_VK_RESULT(m_context->getQueue().submit({submitInfo}, m_frameCtrl->frameDone->get().get()));

    const bool presented = m_frameCtrl->window->present(m_frameCtrl->renderingFinished.get(), view);
    if (!presented)
      nextSwapchainImage = std::nullopt;
  }

  etna::end_frame();

  if (!nextSwapchainImage)
    RecreateSwapChain();
}

void SimpleShadowmapRender::DrawFrame(float a_time, DrawMode a_mode)
{
  UpdateUniformBuffer(a_time);
  switch (a_mode)
  {
    case DrawMode::WITH_GUI:
      DrawGui();
      DrawFrameSimple(true);
      break;
    case DrawMode::NO_GUI:
      DrawFrameSimple(false);
      break;
    default:
      DrawFrameSimple(false);
  }
}
