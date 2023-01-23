#include "shadowmap_render.h"
#include <etna/Etna.hpp>

#include <imgui/imgui.h>

#include "../../render/render_gui.h"

void SimpleShadowmapRender::DrawFrameSimple(bool draw_gui)
{
  vkWaitForFences(m_context->getDevice(), 1, &m_frameFences[m_presentationResources.currentFrame], VK_TRUE, UINT64_MAX);
  vkResetFences(m_context->getDevice(), 1, &m_frameFences[m_presentationResources.currentFrame]);

  uint32_t imageIdx;
  m_swapchain.AcquireNextImage(m_presentationResources.imageAvailable, &imageIdx);

  auto currentCmdBuf = m_cmdBuffersDrawMain[m_presentationResources.currentFrame];

  VkSemaphore waitSemaphores[] = {m_presentationResources.imageAvailable};
  VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};

  BuildCommandBufferSimple(currentCmdBuf, m_swapchain.GetAttachment(imageIdx).image, m_swapchain.GetAttachment(imageIdx).view);

  std::vector<VkCommandBuffer> submitCmdBufs = { currentCmdBuf };

  if (draw_gui)
  {
    ImDrawData* pDrawData = ImGui::GetDrawData();
    auto currentGUICmdBuf = m_pGUIRender->BuildGUIRenderCommand(imageIdx, pDrawData);
    submitCmdBufs.push_back(currentGUICmdBuf);
  }

  VkSubmitInfo submitInfo = {};
  submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
  submitInfo.waitSemaphoreCount = 1;
  submitInfo.pWaitSemaphores = waitSemaphores;
  submitInfo.pWaitDstStageMask = waitStages;
  submitInfo.commandBufferCount = submitCmdBufs.size();
  submitInfo.pCommandBuffers = submitCmdBufs.data();

  VkSemaphore signalSemaphores[] = {m_presentationResources.renderingFinished};
  submitInfo.signalSemaphoreCount = 1;
  submitInfo.pSignalSemaphores = signalSemaphores;

  VK_CHECK_RESULT(vkQueueSubmit(m_context->getQueue(),
    1, &submitInfo, m_frameFences[m_presentationResources.currentFrame]));

  VkResult presentRes = m_swapchain.QueuePresent(m_presentationResources.queue, imageIdx,
                                                 m_presentationResources.renderingFinished);

  if (presentRes == VK_ERROR_OUT_OF_DATE_KHR || presentRes == VK_SUBOPTIMAL_KHR)
  {
    RecreateSwapChain();
  }
  else if (presentRes != VK_SUCCESS)
  {
    RUN_TIME_ERROR("Failed to present swapchain image");
  }

  m_presentationResources.currentFrame = (m_presentationResources.currentFrame + 1) % m_framesInFlight;

  vkQueueWaitIdle(m_presentationResources.queue);
  
  etna::submit();
}

void SimpleShadowmapRender::DrawFrame(float a_time, DrawMode a_mode)
{
  UpdateUniformBuffer(a_time);
  switch (a_mode)
  {
    case DrawMode::WITH_GUI:
      SetupGUIElements();
      DrawFrameSimple(true);
      break;
    case DrawMode::NO_GUI:
      DrawFrameSimple(false);
      break;
    default:
      DrawFrameSimple(false);
  }

}
