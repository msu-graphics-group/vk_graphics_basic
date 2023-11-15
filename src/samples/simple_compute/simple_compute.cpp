#include "simple_compute.h"

#include <vk_pipeline.h>
#include <vk_buffers.h>
#include <vk_utils.h>

#include <etna/Etna.hpp>
#include <vulkan/vulkan_core.h>

SimpleCompute::SimpleCompute(uint32_t a_length) : m_length(a_length) {}

void SimpleCompute::InitVulkan(const char** a_instanceExtensions, uint32_t a_instanceExtensionsCount, uint32_t a_deviceId)
{
  for (uint32_t i = 0; i < a_instanceExtensionsCount; ++i) {
    m_instanceExtensions.push_back(a_instanceExtensions[i]);
  }

  #ifndef NDEBUG
    m_instanceExtensions.push_back("VK_EXT_debug_report");
  #endif
  
  etna::initialize(etna::InitParams
    {
      .applicationName = "ComputeSample",
      .applicationVersion = VK_MAKE_VERSION(0, 1, 0),
      .instanceExtensions = m_instanceExtensions,
      .deviceExtensions = m_deviceExtensions,
      // .physicalDeviceIndexOverride = 0,
    }
  );

  m_context = &etna::get_context();
  m_commandPool = vk_utils::createCommandPool(m_context->getDevice(), m_context->getQueueFamilyIdx(), VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);
  
  m_cmdBufferCompute = vk_utils::createCommandBuffers(m_context->getDevice(), m_commandPool, 1)[0];
  
  m_pCopyHelper = std::make_shared<vk_utils::SimpleCopyHelper>(m_context->getPhysicalDevice(), 
    m_context->getDevice(), m_context->getQueue(), m_context->getQueueFamilyIdx(), 8 * 1024 * 1024);
}

void SimpleCompute::SetupSimplePipeline()
{
  //// Создание буферов
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
      .bufferUsage = vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eTransferSrc,
      .name = "m_sum"
    }
  );
  
  //// Заполнение буферов
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

  //// создание вычислительного конвейера 
  auto &pipelineManager = etna::get_context().getPipelineManager();
  m_pipeline = pipelineManager.createComputePipeline("simple_compute", {});
}

void SimpleCompute::BuildCommandBufferSimple(VkCommandBuffer a_cmdBuff, VkPipeline)
{
  vkResetCommandBuffer(a_cmdBuff, 0);

  VkCommandBufferBeginInfo beginInfo = {};
  beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
  beginInfo.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;

  // Заполняем буфер команд
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


void SimpleCompute::CleanupPipeline()
{ 
  if (m_cmdBufferCompute)
  {
    vkFreeCommandBuffers(m_context->getDevice(), m_commandPool, 1, &m_cmdBufferCompute);
  }

  m_A.~Buffer();
  m_B.~Buffer();
  m_sum.~Buffer();
  vkDestroyFence(m_context->getDevice(), m_fence, nullptr);
}


void SimpleCompute::Cleanup()
{
  CleanupPipeline();

  if (m_commandPool != VK_NULL_HANDLE)
  {
    vkDestroyCommandPool(m_context->getDevice(), m_commandPool, nullptr);
  }
}


void SimpleCompute::loadShaders()
{
  etna::create_program("simple_compute", {"../resources/shaders/simple.comp.spv"});
}


void SimpleCompute::Execute()
{
  loadShaders();
  SetupSimplePipeline();

  BuildCommandBufferSimple(m_cmdBufferCompute, nullptr);

  VkSubmitInfo submitInfo = {};
  submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
  submitInfo.commandBufferCount = 1;
  submitInfo.pCommandBuffers = &m_cmdBufferCompute;

  VkFenceCreateInfo fenceCreateInfo = {};
  fenceCreateInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
  fenceCreateInfo.flags = 0;
  VK_CHECK_RESULT(vkCreateFence(m_context->getDevice(), &fenceCreateInfo, NULL, &m_fence));

  // Отправляем буфер команд на выполнение
  VK_CHECK_RESULT(vkQueueSubmit(m_context->getQueue(), 1, &submitInfo, m_fence));

  //Ждём конца выполнения команд
  VK_CHECK_RESULT(vkWaitForFences(m_context->getDevice(), 1, &m_fence, VK_TRUE, 100000000000));

  std::vector<float> values(m_length);
  m_pCopyHelper->ReadBuffer(m_sum.get(), 0, values.data(), sizeof(float) * values.size());
  for (auto v: values) {
    std::cout << v << ' ';
  }
  std::cout << std::endl;
}
