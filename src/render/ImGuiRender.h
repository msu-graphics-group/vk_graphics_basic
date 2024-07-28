#ifndef VK_GRAPHICS_BASIC_RENDER_GUI_H
#define VK_GRAPHICS_BASIC_RENDER_GUI_H

#include <etna/Vulkan.hpp>


struct ImDrawData;

class ImGuiRender
{
public:
  ImGuiRender(vk::Format a_target_format);

  void NextFrame();

  void Draw(vk::CommandBuffer a_cmdbuf, vk::Rect2D a_rect, vk::Image a_image, vk::ImageView a_view, ImDrawData *a_imgui_draw_data);

  ~ImGuiRender();

private:
  vk::UniqueDescriptorPool m_descriptorPool;

  void InitImGui(vk::Format a_target_format);
  void CleanupImGui();
  void CreateDescriptorPool();
};

#endif// VK_GRAPHICS_BASIC_RENDER_GUI_H