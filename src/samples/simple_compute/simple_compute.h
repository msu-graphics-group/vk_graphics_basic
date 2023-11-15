#ifndef SIMPLE_COMPUTE_H
#define SIMPLE_COMPUTE_H

// #define VK_NO_PROTOTYPES
#include "../../render/compute_common.h"
#include "../resources/shaders/common.h"
#include <vk_descriptor_sets.h>
#include <vk_copy.h>

#include <etna/GlobalContext.hpp>
#include <etna/ComputePipeline.hpp>

#include <string>
#include <iostream>
#include <memory>

class SimpleCompute : public ICompute
{
public:
  SimpleCompute(uint32_t a_length);
  ~SimpleCompute()  { Cleanup(); };

  inline VkInstance GetVkInstance() const override { return m_context->getInstance(); }
  void InitVulkan(const char** a_instanceExtensions, uint32_t a_instanceExtensionsCount, uint32_t a_deviceId) override;

  void Execute() override;

  //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
private:
  etna::GlobalContext *m_context;

  VkCommandPool m_commandPool    = VK_NULL_HANDLE;
  VkCommandBuffer m_cmdBufferCompute = VK_NULL_HANDLE;
  VkFence m_fence;

  uint32_t m_length  = 16u;
  
  VkPhysicalDeviceFeatures m_enabledDeviceFeatures = {};
  std::vector<const char*> m_deviceExtensions      = {};
  std::vector<const char*> m_instanceExtensions    = {};

  std::shared_ptr<vk_utils::ICopyEngine> m_pCopyHelper;
  
  etna::ComputePipeline m_pipeline;

  etna::Buffer m_A, m_B, m_sum;
 
  void BuildCommandBufferSimple(VkCommandBuffer a_cmdBuff, VkPipeline a_pipeline);

  void SetupSimplePipeline();
  void loadShaders();
  void CleanupPipeline();

  void Cleanup();
};


#endif //SIMPLE_COMPUTE_H
