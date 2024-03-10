#include "quad_renderer.h"

#include "etna/GlobalContext.hpp"
#include "etna/Etna.hpp"
#include "etna/RenderTargetStates.hpp"
#include "etna/DescriptorSet.hpp"


QuadRenderer::QuadRenderer(CreateInfo info) 
{
  m_rect      = info.rect;
  m_programId = etna::create_program("quad_renderer", {
    VK_GRAPHICS_BASIC_ROOT "/resources/shaders/quad3_vert.vert.spv",
    VK_GRAPHICS_BASIC_ROOT "/resources/shaders/quad.frag.spv"
  });

  auto &pipelineManager = etna::get_context().getPipelineManager();
  m_pipeline = pipelineManager.createGraphicsPipeline("quad_renderer",
    {
      .fragmentShaderOutput = 
      {
        .colorAttachmentFormats = {info.format}
      }
    });
}

void QuadRenderer::RecordCommands(vk::CommandBuffer cmdBuff, vk::Image targetImage, vk::ImageView targetImageView,
                                  const etna::Image &inTex, const etna::Sampler &sampler)
{
  auto programInfo = etna::get_shader_program(m_programId);
  auto set = etna::create_descriptor_set(programInfo.getDescriptorLayoutId(0), cmdBuff,
    {
      etna::Binding {0, inTex.genBinding(sampler.get(), vk::ImageLayout::eShaderReadOnlyOptimal)}
    });
  vk::DescriptorSet vkSet = set.getVkSet();

  etna::RenderTargetState renderTargets(cmdBuff, m_rect, {{.image = targetImage, .view = targetImageView, .loadOp = vk::AttachmentLoadOp::eLoad}}, {});

  cmdBuff.bindPipeline(vk::PipelineBindPoint::eGraphics, m_pipeline.getVkPipeline());
  cmdBuff.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, m_pipeline.getVkPipelineLayout(), 0, {vkSet}, {});

  cmdBuff.draw(3, 1, 0, 0);
}
