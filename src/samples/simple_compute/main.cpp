#include "simple_compute.h"
#include <etna/Etna.hpp>

int main()
{
  constexpr int LENGTH = 10;

  std::shared_ptr<ICompute> app = std::make_unique<SimpleCompute>(LENGTH);
  if(app == nullptr)
  {
    std::cout << "Can't create render of specified type" << std::endl;
    return 1;
  }

  app->InitVulkan(nullptr, 0, /* useless, check etna::initialize for device override */ 0);

  app->Execute();

  app = {};
  if (etna::is_initilized())
    etna::shutdown();

  return 0;
}
