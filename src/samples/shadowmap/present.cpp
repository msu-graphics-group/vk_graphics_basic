#include "shadowmap_render.h"

#include "../../render/ImGuiRender.h"


void SimpleShadowmapRender::InitPresentation(vk::UniqueSurfaceKHR a_surface, etna::ResolutionProvider a_res_provider)
{
  commandManager = m_context->createCommandManager();

  window = m_context->createWindow(etna::Window::CreateInfo{
    .surface            = std::move(a_surface),
    .resolutionProvider = std::move(a_res_provider),
  });

  auto [w, h] = window->recreateSwapchain();
  m_width     = w;
  m_height    = h;

  m_pGUIRender = std::make_unique<ImGuiRender>(window->getCurrentFormat());

  AllocateResources();
}
