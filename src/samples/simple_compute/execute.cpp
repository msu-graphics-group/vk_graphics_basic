#include "simple_compute.h"

#include <etna/Etna.hpp>


void SimpleCompute::Execute()
{
  Setup();

  auto cmdBuf = cmdMgr->start();

  BuildCommandBuffer(cmdBuf);

  cmdMgr->submitAndWait(std::move(cmdBuf));

  Readback();
}
