#ifndef SIMPLE_COMPUTE_H
#define SIMPLE_COMPUTE_H

#include <memory>

#include <etna/GlobalContext.hpp>
#include <etna/ComputePipeline.hpp>
#include <etna/OneShotCmdMgr.hpp>
#include <etna/BlockingTransferHelper.hpp>


class SimpleCompute
{
public:
  SimpleCompute(std::uint32_t len);

  void Init();
  void Execute();

  //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
private:
  etna::GlobalContext *context;

  std::unique_ptr<etna::OneShotCmdMgr> cmdMgr;
  std::unique_ptr<etna::BlockingTransferHelper> transferHelper;

  std::uint32_t length;

  etna::ComputePipeline pipeline;

  etna::Buffer bufA;
  etna::Buffer bufB;
  etna::Buffer bufResult;

  void Setup();
  void BuildCommandBuffer(vk::CommandBuffer cmd_buf);
  void Readback();
};


#endif //SIMPLE_COMPUTE_H
