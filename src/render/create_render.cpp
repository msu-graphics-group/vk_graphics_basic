#include "render_common.h"
#include "simple_render.h"
#include "shadowmap_render.h"


std::unique_ptr<IRender> CreateRender(uint32_t w, uint32_t h, RenderEngineType type)
{
  switch(type)
  {
  case RenderEngineType::SIMPLE_FORWARD:
    return std::make_unique<SimpleRender>(w, h);

  case RenderEngineType::SIMPLE_SHADOWMAP:
    return std::make_unique<SimpleShadowmapRender>(w, h);

  default:
    return nullptr;
  }
  return nullptr;
}


