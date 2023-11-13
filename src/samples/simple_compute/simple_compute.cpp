#include "simple_compute.h"

#include <vk_pipeline.h>
#include <vk_buffers.h>
#include <vk_utils.h>

#include <cstdlib>
#include <chrono>

SimpleCompute::SimpleCompute(uint32_t a_length) : m_length(a_length)
{
#ifdef NDEBUG
  m_enableValidation = false;
#else
  m_enableValidation = true;
#endif
}

void SimpleCompute::SetupValidationLayers()
{
  m_validationLayers.push_back("VK_LAYER_KHRONOS_validation");
  m_validationLayers.push_back("VK_LAYER_LUNARG_monitor");
}

void SimpleCompute::InitVulkan(const char **a_instanceExtensions, uint32_t a_instanceExtensionsCount, uint32_t a_deviceId)
{
  m_instanceExtensions.clear();
  for (uint32_t i = 0; i < a_instanceExtensionsCount; ++i)
  {
    m_instanceExtensions.push_back(a_instanceExtensions[i]);
  }
  SetupValidationLayers();
  VK_CHECK_RESULT(volkInitialize());
  CreateInstance();
  volkLoadInstance(m_instance);

  CreateDevice(a_deviceId);
  volkLoadDevice(m_device);

  m_commandPool = vk_utils::createCommandPool(m_device, m_queueFamilyIDXs.compute, VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);

  m_cmdBufferCompute = vk_utils::createCommandBuffers(m_device, m_commandPool, 1)[0];

  m_pCopyHelper = std::make_shared<vk_utils::SimpleCopyHelper>(m_physicalDevice, m_device, m_transferQueue, m_queueFamilyIDXs.compute, 8 * 1024 * 1024);
}


void SimpleCompute::CreateInstance()
{
  VkApplicationInfo appInfo  = {};
  appInfo.sType              = VK_STRUCTURE_TYPE_APPLICATION_INFO;
  appInfo.pNext              = nullptr;
  appInfo.pApplicationName   = "VkRender";
  appInfo.applicationVersion = VK_MAKE_VERSION(0, 1, 0);
  appInfo.pEngineName        = "SimpleCompute";
  appInfo.engineVersion      = VK_MAKE_VERSION(0, 1, 0);
  appInfo.apiVersion         = VK_MAKE_VERSION(1, 1, 0);

  m_instance = vk_utils::createInstance(m_enableValidation, m_validationLayers, m_instanceExtensions, &appInfo);
  if (m_enableValidation)
    vk_utils::initDebugReportCallback(m_instance, &debugReportCallbackFn, &m_debugReportCallback);
}

void SimpleCompute::CreateDevice(uint32_t a_deviceId)
{
  m_physicalDevice = vk_utils::findPhysicalDevice(m_instance, true, a_deviceId, m_deviceExtensions);

  m_device = vk_utils::createLogicalDevice(m_physicalDevice, m_validationLayers, m_deviceExtensions, m_enabledDeviceFeatures, m_queueFamilyIDXs, VK_QUEUE_COMPUTE_BIT | VK_QUEUE_TRANSFER_BIT);

  vkGetDeviceQueue(m_device, m_queueFamilyIDXs.compute, 0, &m_computeQueue);
  vkGetDeviceQueue(m_device, m_queueFamilyIDXs.transfer, 0, &m_transferQueue);
}

float randomFloat()
{
  return (float)(rand()) / (float)(RAND_MAX);
}

void SimpleCompute::SetupSimplePipeline()
{
  std::vector<std::pair<VkDescriptorType, uint32_t>> dtypes = {
    { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 2 }
  };

  // Создание и аллокация буферов
  m_Basic   = vk_utils::createBuffer(m_device, sizeof(float) * m_length, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT);
  m_Updated = vk_utils::createBuffer(m_device, sizeof(float) * m_length, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT);

  vk_utils::allocateAndBindWithPadding(m_device, m_physicalDevice, { m_Basic, m_Updated }, 0);

  m_pBindings = std::make_shared<vk_utils::DescriptorMaker>(m_device, dtypes, 1);

  // Создание descriptor set для передачи буферов в шейдер
  m_pBindings->BindBegin(VK_SHADER_STAGE_COMPUTE_BIT);
  m_pBindings->BindBuffer(0, m_Basic);
  m_pBindings->BindBuffer(1, m_Updated);

  m_pBindings->BindEnd(&m_sumDS, &m_sumDSLayout);

  srand(time(nullptr));
  // Заполнение буферов
  m_array.resize(m_length);
  for (float &i : m_array)
  {
    i = randomFloat();
  }
  m_pCopyHelper->UpdateBuffer(m_Basic, 0, m_array.data(), sizeof(float) * m_array.size());
  m_pCopyHelper->UpdateBuffer(m_Updated, 0, m_array.data(), sizeof(float) * m_array.size());
}

void SimpleCompute::BuildCommandBufferSimple(VkCommandBuffer a_cmdBuff, VkPipeline)
{
  vkResetCommandBuffer(a_cmdBuff, 0);

  VkCommandBufferBeginInfo beginInfo = {};
  beginInfo.sType                    = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
  beginInfo.flags                    = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;

  // Заполняем буфер команд
  VK_CHECK_RESULT(vkBeginCommandBuffer(a_cmdBuff, &beginInfo));

  vkCmdBindPipeline(a_cmdBuff, VK_PIPELINE_BIND_POINT_COMPUTE, m_pipeline);
  vkCmdBindDescriptorSets(a_cmdBuff, VK_PIPELINE_BIND_POINT_COMPUTE, m_layout, 0, 1, &m_sumDS, 0, NULL);

  vkCmdPushConstants(a_cmdBuff, m_layout, VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(m_length), &m_length);

  vkCmdDispatch(a_cmdBuff, 1 + (m_length - 1) / 256, 1, 1);

  VK_CHECK_RESULT(vkEndCommandBuffer(a_cmdBuff));
}


void SimpleCompute::CleanupPipeline()
{
  if (m_cmdBufferCompute)
  {
    vkFreeCommandBuffers(m_device, m_commandPool, 1, &m_cmdBufferCompute);
  }

  vkDestroyBuffer(m_device, m_Basic, nullptr);
  vkDestroyBuffer(m_device, m_Updated, nullptr);

  vkDestroyPipelineLayout(m_device, m_layout, nullptr);
  vkDestroyPipeline(m_device, m_pipeline, nullptr);
}


void SimpleCompute::Cleanup()
{
  CleanupPipeline();

  if (m_commandPool != VK_NULL_HANDLE)
  {
    vkDestroyCommandPool(m_device, m_commandPool, nullptr);
  }
}


void SimpleCompute::CreateComputePipeline()
{
  // Загружаем шейдер
  std::vector<uint32_t> code          = vk_utils::readSPVFile("../resources/shaders/simple.comp.spv");
  VkShaderModuleCreateInfo createInfo = {};
  createInfo.sType                    = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
  createInfo.pCode                    = code.data();
  createInfo.codeSize                 = code.size() * sizeof(uint32_t);

  VkShaderModule shaderModule;
  // Создаём шейдер в вулкане
  VK_CHECK_RESULT(vkCreateShaderModule(m_device, &createInfo, NULL, &shaderModule));

  VkPipelineShaderStageCreateInfo shaderStageCreateInfo = {};
  shaderStageCreateInfo.sType                           = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
  shaderStageCreateInfo.stage                           = VK_SHADER_STAGE_COMPUTE_BIT;
  shaderStageCreateInfo.module                          = shaderModule;
  shaderStageCreateInfo.pName                           = "main";

  VkPushConstantRange pcRange = {};
  pcRange.offset              = 0;
  pcRange.size                = sizeof(m_length);
  pcRange.stageFlags          = VK_SHADER_STAGE_COMPUTE_BIT;

  // Создаём layout для pipeline
  VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo = {};
  pipelineLayoutCreateInfo.sType                      = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
  pipelineLayoutCreateInfo.setLayoutCount             = 1;
  pipelineLayoutCreateInfo.pSetLayouts                = &m_sumDSLayout;
  pipelineLayoutCreateInfo.pushConstantRangeCount     = 1;
  pipelineLayoutCreateInfo.pPushConstantRanges        = &pcRange;
  VK_CHECK_RESULT(vkCreatePipelineLayout(m_device, &pipelineLayoutCreateInfo, NULL, &m_layout));

  VkComputePipelineCreateInfo pipelineCreateInfo = {};
  pipelineCreateInfo.sType                       = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
  pipelineCreateInfo.stage                       = shaderStageCreateInfo;
  pipelineCreateInfo.layout                      = m_layout;

  // Создаём pipeline - объект, который выставляет шейдер и его параметры
  VK_CHECK_RESULT(vkCreateComputePipelines(m_device, VK_NULL_HANDLE, 1, &pipelineCreateInfo, NULL, &m_pipeline));

  vkDestroyShaderModule(m_device, shaderModule, nullptr);
}


void SimpleCompute::Execute()
{
  SetupSimplePipeline();
  CreateComputePipeline();

  BuildCommandBufferSimple(m_cmdBufferCompute, nullptr);

  VkSubmitInfo submitInfo       = {};
  submitInfo.sType              = VK_STRUCTURE_TYPE_SUBMIT_INFO;
  submitInfo.commandBufferCount = 1;
  submitInfo.pCommandBuffers    = &m_cmdBufferCompute;

  VkFenceCreateInfo fenceCreateInfo = {};
  fenceCreateInfo.sType             = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
  fenceCreateInfo.flags             = 0;
  VK_CHECK_RESULT(vkCreateFence(m_device, &fenceCreateInfo, NULL, &m_fence));

  auto startShader = std::chrono::high_resolution_clock::now();
  // Отправляем буфер команд на выполнение
  VK_CHECK_RESULT(vkQueueSubmit(m_computeQueue, 1, &submitInfo, m_fence));

  // Ждём конца выполнения команд
  VK_CHECK_RESULT(vkWaitForFences(m_device, 1, &m_fence, VK_TRUE, 100000000000));

  std::vector<float> values(m_length);
  m_pCopyHelper->ReadBuffer(m_Updated, 0, values.data(), sizeof(float) * values.size());

  float res = 0.0;
  for (auto v : values)
  {
    res += v;
  }

  auto timeShader             = std::chrono::high_resolution_clock::now() - startShader;
  auto timeShaderMilliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(timeShader);

  std::cout << "Result using shader: " << res << "\n";
  std::cout << "Time using shader(ms): " << timeShaderMilliseconds.count() << "\n";

  auto startCPU = std::chrono::high_resolution_clock::now();

  std::vector<float> new_values(m_length);

  for (uint idx = 0; idx < m_array.size(); idx++)
  {
    int idx_min = std::max(0, (int)idx - 3);
    int idx_max = std::min((int)idx + 4, (int)m_array.size());

    new_values[idx] = m_array[idx];
    for (int i = idx_min; i < idx_max; ++i)
    {
      new_values[idx] -= (m_array[i] / (float)7.0);
    }
  }

  res = 0.0;
  for (auto v : new_values)
  {
    res += v;
  }

  auto timeCPU             = std::chrono::high_resolution_clock::now() - startCPU;
  auto timeCPUMilliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(timeCPU);

  std::cout << "Result using c++: " << res << "\n";
  std::cout << "Time using c++(ms): " << timeCPUMilliseconds.count() << "\n";
}
