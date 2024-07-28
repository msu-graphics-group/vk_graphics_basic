#ifndef SIMPLE_COMPUTE_H
#define SIMPLE_COMPUTE_H

#include <memory>

#include <etna/GlobalContext.hpp>
#include <etna/ComputePipeline.hpp>
#include <vk_copy.h>


class SimpleCompute
{
public:
  SimpleCompute(uint32_t a_length);

  void Init();
  void Execute();

  //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
private:
  etna::GlobalContext *m_context;

  vk::UniqueCommandPool m_commandPool;
  vk::UniqueCommandBuffer m_cmdBufferCompute;
  vk::UniqueFence m_fence;

  std::uint32_t m_length;

  std::unique_ptr<vk_utils::ICopyEngine> m_pCopyHelper;

  etna::ComputePipeline m_pipeline;

  etna::Buffer m_A, m_B, m_sum;

  void Setup();
  void BuildCommandBuffer(vk::CommandBuffer a_cmdBuff);
};


#endif //SIMPLE_COMPUTE_H
