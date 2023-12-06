#include "simple_compute.h"

#include <vk_pipeline.h>
#include <vk_buffers.h>
#include <vk_utils.h>

#include <etna/Etna.hpp>
#include <vulkan/vulkan_core.h>


void SimpleCompute::loadShaders()
{
  etna::create_program("simple_compute", {VK_GRAPHICS_BASIC_ROOT"/resources/shaders/simple.comp.spv"});
}

void SimpleCompute::SetupSimplePipeline()
{
  //// Buffer creation
  //
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
  // 
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

void SimpleCompute::CleanupPipeline()
{ 
  if (m_cmdBufferCompute)
  {
    vkFreeCommandBuffers(m_context->getDevice(), m_commandPool, 1, &m_cmdBufferCompute);
  }

  m_A.reset();
  m_B.reset();
  m_sum.reset();
  vkDestroyFence(m_context->getDevice(), m_fence, nullptr);
}

void SimpleCompute::BuildCommandBufferSimple(VkCommandBuffer a_cmdBuff, VkPipeline)
{
  vkResetCommandBuffer(a_cmdBuff, 0);

  VkCommandBufferBeginInfo beginInfo = {};
  beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
  beginInfo.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;

  // Filling the command buffer
  VK_CHECK_RESULT(vkBeginCommandBuffer(a_cmdBuff, &beginInfo));

  auto simpleComputeInfo = etna::get_shader_program("simple_compute");

  auto set = etna::create_descriptor_set(simpleComputeInfo.getDescriptorLayoutId(0), a_cmdBuff,
    {
      etna::Binding {0, m_A.genBinding()},
      etna::Binding {1, m_B.genBinding()},
      etna::Binding {2, m_sum.genBinding()},
    }
  );

  VkDescriptorSet vkSet = set.getVkSet();

  vkCmdBindPipeline      (a_cmdBuff, VK_PIPELINE_BIND_POINT_COMPUTE, m_pipeline.getVkPipeline());
  vkCmdBindDescriptorSets(a_cmdBuff, VK_PIPELINE_BIND_POINT_COMPUTE, m_pipeline.getVkPipelineLayout(), 0, 1, &vkSet, 0, NULL);

  vkCmdPushConstants(a_cmdBuff, m_pipeline.getVkPipelineLayout(), VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(m_length), &m_length);

  etna::flush_barriers(a_cmdBuff);

  vkCmdDispatch(a_cmdBuff, 1, 1, 1);

  VK_CHECK_RESULT(vkEndCommandBuffer(a_cmdBuff));
}
