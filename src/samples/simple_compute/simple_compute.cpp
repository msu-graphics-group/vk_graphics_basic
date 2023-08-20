#include "simple_compute.h"

#include <vk_pipeline.h>
#include <vk_buffers.h>
#include <vk_utils.h>

#include <etna/Etna.hpp>
#include <vulkan/vulkan_core.h>

SimpleCompute::SimpleCompute(uint32_t a_length) : m_length(a_length) {}

void SimpleCompute::InitVulkan(const char** a_instanceExtensions, uint32_t a_instanceExtensionsCount, uint32_t a_deviceId)
{
  m_instanceExtensions.clear();
  for (uint32_t i = 0; i < a_instanceExtensionsCount; ++i) {
    m_instanceExtensions.push_back(a_instanceExtensions[i]);
  }
  
  etna::initialize(etna::InitParams
    {
      .applicationName = "ComputeSample",
      .applicationVersion = VK_MAKE_VERSION(0, 1, 0),
      .instanceExtensions = m_instanceExtensions,
      .deviceExtensions = m_deviceExtensions,
      // .physicalDeviceIndexOverride = 1,
    }
  );

  m_context = &etna::get_context();
  m_commandPool = vk_utils::createCommandPool(m_context->getDevice(), m_context->getQueueFamilyIdx(), VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);
  
  m_cmdBufferCompute = vk_utils::createCommandBuffers(m_context->getDevice(), m_commandPool, 1)[0];
  
  m_pCopyHelper = std::make_shared<vk_utils::SimpleCopyHelper>(m_context->getPhysicalDevice(), m_context->getDevice(), m_context->getQueue(), m_context->getQueueFamilyIdx(), 8*1024*1024);
}

void SimpleCompute::SetupSimplePipeline()
{
  std::vector<std::pair<VkDescriptorType, uint32_t> > dtypes = {
      {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,             3}
  };

  // Создание и аллокация буферов
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
  
  m_pBindings = std::make_shared<vk_utils::DescriptorMaker>(m_context->getDevice(), dtypes, 1);

  // Создание descriptor set для передачи буферов в шейдер
  m_pBindings->BindBegin(VK_SHADER_STAGE_COMPUTE_BIT);
  m_pBindings->BindBuffer(0, m_A.get());
  m_pBindings->BindBuffer(1, m_B.get());
  m_pBindings->BindBuffer(2, m_sum.get());
  m_pBindings->BindEnd(&m_sumDS, &m_sumDSLayout);

  // Заполнение буферов
  std::vector<float> values(m_length);
  for (uint32_t i = 0; i < values.size(); ++i) {
    values[i] = (float)i;
  }
  m_pCopyHelper->UpdateBuffer(m_A.get(), 0, values.data(), sizeof(float) * values.size());
  for (uint32_t i = 0; i < values.size(); ++i) {
    values[i] = (float)i * i;
  }
  m_pCopyHelper->UpdateBuffer(m_B.get(), 0, values.data(), sizeof(float) * values.size());

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

  vkCmdBindPipeline      (a_cmdBuff, VK_PIPELINE_BIND_POINT_COMPUTE, m_pipeline.getVkPipeline());
  vkCmdBindDescriptorSets(a_cmdBuff, VK_PIPELINE_BIND_POINT_COMPUTE, m_pipeline.getVkPipelineLayout(), 0, 1, &m_sumDS, 0, NULL);

  vkCmdPushConstants(a_cmdBuff, m_pipeline.getVkPipelineLayout(), VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(m_length), &m_length);

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
