#include "simple_compute.h"

#include <etna/Etna.hpp>


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

  // Submit the command buffer for execution
  VK_CHECK_RESULT(vkQueueSubmit(m_context->getQueue(), 1, &submitInfo, m_fence));

  // And wait for the execution to be completed
  VK_CHECK_RESULT(vkWaitForFences(m_context->getDevice(), 1, &m_fence, VK_TRUE, UINT64_MAX));

  std::vector<float> values(m_length);
  m_pCopyHelper->ReadBuffer(m_sum.get(), 0, values.data(), sizeof(float) * values.size());

  std::cout << std::endl;
  for (auto v: values) 
  {
    std::cout << v << ' ';
  }
  std::cout << std::endl << std::endl;
}
