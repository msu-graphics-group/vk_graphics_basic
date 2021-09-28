#ifndef VK_GRAPHICS_BASIC_CREATE_RENDER_H
#define VK_GRAPHICS_BASIC_CREATE_RENDER_H

enum class RenderEngineType
{
  SIMPLE_FORWARD,
  SIMPLE_TEXTURE
};

std::unique_ptr<IRender> CreateRender(uint32_t w, uint32_t h, RenderEngineType type);

#endif// VK_GRAPHICS_BASIC_CREATE_RENDER_H
