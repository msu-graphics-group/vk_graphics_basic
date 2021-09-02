#include "render/render_common.h"
#include "glfw_window.h"


int main()
{
  std::shared_ptr<IRender> app = CreateRender(2048, 2048, RenderEngineType::SIMPLE_FORWARD);
  if(app == nullptr)
  {
    std::cout << "Can't create render of specified type\n";
    return 1;
  }
  auto* window = Init(app, 0);

  app->LoadScene("../resources/scenes/043_cornell_normals/statex_00001.xml", false);

  MainLoop(app, window);

  return 0;
}
