#include "simple_compute.h"
#include <etna/Etna.hpp>

int main()
{
  constexpr int LENGTH = 10;

  {
    SimpleCompute app(LENGTH);

    app.Init();
    app.Execute();
  }

  if (etna::is_initilized())
    etna::shutdown();

  return 0;
}
