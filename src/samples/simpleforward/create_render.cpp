#include "render/render_common.h"
#include "create_render.h"
#include "simple_render.h"
#include "simple_render_tex.h"


std::unique_ptr<IRender> CreateRender(uint32_t w, uint32_t h, RenderEngineType type)
{
  switch(type)
  {
  case RenderEngineType::SIMPLE_FORWARD:
    return std::make_unique<SimpleRender>(w, h);
  
  case RenderEngineType::SIMPLE_TEXTURE:
    return std::make_unique<SimpleRenderTexture>(w, h);

  default:
    return nullptr;
  }
}


