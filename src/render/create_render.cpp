#include "render_common.h"
#include "simple_render.h"
#include "shadowmap_render.h"
#include "quad2d_render.h"
#include "simple_render_tex.h"


std::unique_ptr<IRender> CreateRender(uint32_t w, uint32_t h, RenderEngineType type)
{
  switch(type)
  {
  case RenderEngineType::SIMPLE_FORWARD:
    return std::make_unique<SimpleRender>(w, h);

  case RenderEngineType::SIMPLE_SHADOWMAP:
    return std::make_unique<SimpleShadowmapRender>(w, h);
  
  case RenderEngineType::SIMPLE_QUAD2D:
    return std::make_unique<Quad2D_Render>(w, h);

  case RenderEngineType::SIMPLE_TEXTURE:
    return std::make_unique<SimpleRenderTexture>(w, h);
      
  default:
    return nullptr;
  }
  return nullptr;
}


