#include "shadowmap_render.h"
#include <etna/Etna.hpp>

#include <imgui/imgui.h>
#include "../../render/ImGuiRender.h"


void SimpleShadowmapRender::DrawFrameSimple(bool draw_gui)
{
  auto currentCmdBuf = commandManager->acquireNext();

  // TODO: this makes literally 0 sense here, rename/refactor,
  // it doesn't actually begin anything, just resets descriptor pools
  etna::begin_frame();


  auto nextSwapchainImage = window->acquireNext();

  // NOTE: here, we skip frames when the window is in the process of being
  // re-sized. This is not mandatory, it is possible to submit frames to a
  // "sub-optimal" swap chain and still get something drawn while resizing,
  // but only on some platforms (not windows+nvidia, sadly).
  if (nextSwapchainImage)
  {
    auto[image, view, availableSem] = *nextSwapchainImage;

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

    auto renderingDone = commandManager->submit(currentCmdBuf, availableSem);

    const bool presented = window->present(renderingDone, view);

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
