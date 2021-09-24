#include "render/render_common.h"
#include "glfw_window.h"

int main()
{
  std::shared_ptr<IRender> app = CreateRender(1024, 1024, RenderEngineType::SIMPLE_QUAD2D);
  auto* window = Init(app, 0);

  app->LoadScene("../resources/scenes/043_cornell_normals/statex_00001.xml", false);

  MainLoop(app, window);

  return 0;
}
