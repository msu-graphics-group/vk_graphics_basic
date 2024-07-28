#include "simple_compute.h"

#include <etna/Etna.hpp>
#include <fmt/ranges.h> // NOTE: vector and co are only printable with this included


void SimpleCompute::Execute()
{
  Setup();

  auto cmdBuf = m_cmdBufferCompute.get();

  BuildCommandBuffer(cmdBuf);

  vk::SubmitInfo submitInfo{
    .commandBufferCount = 1,
    .pCommandBuffers = &cmdBuf,
  };

  // Submit the command buffer for execution
  ETNA_CHECK_VK_RESULT(m_context->getQueue().submit({submitInfo}, m_fence.get()));

  // And wait for the execution to be completed
  ETNA_CHECK_VK_RESULT(m_context->getDevice().waitForFences({m_fence.get()}, vk::True, 100000000));

  std::vector<float> values(m_length);
  m_pCopyHelper->ReadBuffer(m_sum.get(), 0, values.data(), sizeof(float) * values.size());

  spdlog::info("Result on cpu:\n{}", values);
}
