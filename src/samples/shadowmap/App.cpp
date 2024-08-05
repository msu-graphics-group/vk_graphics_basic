#include "App.hpp"

#include "render/ImGuiRender.h"


App::App()
{
  glm::uvec2 initialRes = {1280, 720};
  mainWindow = windowing.createWindow(initialRes);

  renderer.reset(new SimpleShadowmapRender(initialRes));

  auto instExts = windowing.getRequiredVulkanInstanceExtensions();
  renderer->initVulkan(instExts);

  auto surface = mainWindow->createVkSurface(etna::get_context().getInstance());

  renderer->initPresentation(std::move(surface), [window = mainWindow.get()]() -> vk::Extent2D {
      auto res = window->getResolution();
      ETNA_ASSERTF(res.x > 0 && res.y > 0, "Got a 0 in window resolution! Is the window minimized?");
      return {static_cast<uint32_t>(res.x), static_cast<uint32_t>(res.y)};
  });

  // TODO: this is bad design, this initialization is dependent on the current ImGui context, but we pass
  // it implicitly here instead of explicitly. Beware if trying to do something tricky.
  ImGuiRender::enableImGuiForWindow(mainWindow->native());

  shadowCam.lookAt({4, 4, 4}, {0, 0, 0}, {0, 1, 0});
  mainCam.lookAt({0, 0, 4}, {0, 0, 0}, {0, 1, 0});

  renderer->loadScene(VK_GRAPHICS_BASIC_ROOT "/resources/scenes/043_cornell_normals/statex_00001.xml", false);
}

void App::run()
{
  double lastTime = windowing.getTime();
  while (!mainWindow->isBeingClosed())
  {
    const double thisTime = windowing.getTime();
    const float diffTime = static_cast<float>(thisTime - lastTime);
    lastTime = thisTime;

    windowing.poll();

    processInput(diffTime);

    renderer->debugInput(mainWindow->keyboard);
    renderer->updateView(mainCam, shadowCam);

    renderer->drawFrame(diffTime);
  }
}

void App::processInput(float dt)
{
  if (mainWindow->keyboard[KeyboardKey::kEscape] == ButtonState::Falling)
    mainWindow->askToClose();

  if (is_held_down(mainWindow->keyboard[KeyboardKey::kLeftShift]))
    camMoveSpeed = 10;
  else
    camMoveSpeed = 1;

  if (mainWindow->keyboard[KeyboardKey::kL] == ButtonState::Falling)
    controlShadowCam = !controlShadowCam;

  if (mainWindow->mouse[MouseButton::mbRight] == ButtonState::Rising)
    mainWindow->captureMouse = !mainWindow->captureMouse;

  auto &camToControl = controlShadowCam ? shadowCam : mainCam;

  moveCam(camToControl, mainWindow->keyboard, dt);
  if (mainWindow->captureMouse)
    rotateCam(camToControl, mainWindow->mouse, dt);
}

void App::moveCam(Camera& cam, const Keyboard& kb, float dt)
{
  // Move position of camera based on WASD keys, and FR keys for up and down

  glm::vec3 dir = {0, 0, 0};

  if (is_held_down(kb[KeyboardKey::kS]))
    dir -= cam.forward();

  if (is_held_down(kb[KeyboardKey::kW]))
    dir += cam.forward();

  if (is_held_down(kb[KeyboardKey::kA]))
    dir -= cam.right();

  if (is_held_down(kb[KeyboardKey::kD]))
    dir += cam.right();

  if (is_held_down(kb[KeyboardKey::kF]))
    dir -= cam.up();

  if (is_held_down(kb[KeyboardKey::kR]))
    dir += cam.up();

  // NOTE: This is how you make moving diagonally not be faster than
  // in a straight line.
  cam.move(dt * camMoveSpeed * (length(dir) > 1e-9 ? normalize(dir) : dir));
}

void App::rotateCam(Camera& cam, const Mouse& ms, float /*dt*/)
{
  // TODO: should dt be accounted for here?

  // Rotate camera based on mouse movement
  cam.rotate(camRotateSpeed * ms.posDelta.y, camRotateSpeed * ms.posDelta.x);

  // Increase or decrease field of view based on mouse wheel
  cam.fov -= zoomSensitivity * ms.scrollDelta.y;
  if (cam.fov < 1.0f)
    cam.fov = 1.0f;
  if (cam.fov > 180.0f)
    cam.fov = 180.0f;
}
