#include "simple_compute.h"

#include <etna/Etna.hpp>
#include <etna/PipelineManager.hpp>


void SimpleCompute::Setup()
{
  etna::create_program("simple_compute", {VK_GRAPHICS_BASIC_ROOT"/resources/shaders/simple.comp.spv"});

  //// Buffer creation

  m_A = m_context->createBuffer(etna::Buffer::CreateInfo
    {
      .size = sizeof(float) * m_length,
      .bufferUsage = vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eTransferDst,
      .name = "m_A"
    }
  );

  m_B = m_context->createBuffer(etna::Buffer::CreateInfo
    {
      .size = sizeof(float) * m_length,
      .bufferUsage = vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eTransferDst,
      .name = "m_B"
    }
  );

  m_sum = m_context->createBuffer(etna::Buffer::CreateInfo
    {
      .size = sizeof(float) * m_length,
      .bufferUsage = vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eTransferSrc,
      .name = "m_sum"
    }
  );

  //// Filling the buffers

  std::vector<float> values(m_length);
  for (uint32_t i = 0; i < values.size(); ++i) {
    values[i] = (float)i;
  }
  m_pCopyHelper->UpdateBuffer(m_A.get(), 0, values.data(), sizeof(float) * values.size());

  for (uint32_t i = 0; i < values.size(); ++i) {
    values[i] = (float)i * i;
  }
  m_pCopyHelper->UpdateBuffer(m_B.get(), 0, values.data(), sizeof(float) * values.size());

  //// Compute pipeline creation
  auto &pipelineManager = etna::get_context().getPipelineManager();
  m_pipeline = pipelineManager.createComputePipeline("simple_compute", {});
}

void SimpleCompute::BuildCommandBuffer(vk::CommandBuffer a_cmdBuff)
{
  a_cmdBuff.reset({});

  ETNA_CHECK_VK_RESULT(a_cmdBuff.begin(vk::CommandBufferBeginInfo{}));

  auto simpleComputeInfo = etna::get_shader_program("simple_compute");

  auto set = etna::create_descriptor_set(simpleComputeInfo.getDescriptorLayoutId(0), a_cmdBuff,
    {
      etna::Binding {0, m_A.genBinding()},
      etna::Binding {1, m_B.genBinding()},
      etna::Binding {2, m_sum.genBinding()},
    }
  );

  vk::DescriptorSet vkSet = set.getVkSet();

  a_cmdBuff.bindPipeline(vk::PipelineBindPoint::eCompute, m_pipeline.getVkPipeline());
  a_cmdBuff.bindDescriptorSets(vk::PipelineBindPoint::eCompute, m_pipeline.getVkPipelineLayout(), 0, 1, &vkSet, 0, NULL);

  a_cmdBuff.pushConstants(m_pipeline.getVkPipelineLayout(), vk::ShaderStageFlagBits::eCompute, 0, sizeof(m_length), &m_length);

  etna::flush_barriers(a_cmdBuff);

  a_cmdBuff.dispatch(1, 1, 1);

  ETNA_CHECK_VK_RESULT(a_cmdBuff.end());
}
