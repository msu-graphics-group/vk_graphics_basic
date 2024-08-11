#pragma once

#include <etna/Vulkan.hpp>
#include <etna/GraphicsPipeline.hpp>
#include <etna/Image.hpp>
#include <etna/Sampler.hpp>


/**
 * Simple class for displaying a texture on the screen for debug purposes.
 */
class QuadRenderer
{
public:
  struct CreateInfo
  {
    vk::Format format = vk::Format::eUndefined;
    vk::Rect2D rect   = {};
  };

  QuadRenderer(CreateInfo info);
  ~QuadRenderer() {}

  void render(vk::CommandBuffer cmdBuff, vk::Image targetImage, vk::ImageView targetImageView,
                      const etna::Image &inTex, const etna::Sampler &sampler);

private:
  etna::GraphicsPipeline m_pipeline;
  etna::ShaderProgramId m_programId;
  vk::Rect2D m_rect {};

  QuadRenderer(const QuadRenderer &) = delete;
  QuadRenderer& operator=(const QuadRenderer &) = delete;
};
