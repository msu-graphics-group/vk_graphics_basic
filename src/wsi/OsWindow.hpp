#pragma once

#include <etna/Vulkan.hpp>

#include "wsi/Keyboard.hpp"
#include "wsi/Mouse.hpp"


struct GLFWwindow;
class OsWindowingManager;

class OsWindow
{
  friend class OsWindowingManager;

  OsWindow(OsWindowingManager* mgr, GLFWwindow* window) : owner{mgr}, impl{window} {}

public:
  OsWindow(const OsWindow&) = delete;
  OsWindow& operator=(const OsWindow&) = delete;

  OsWindow(OsWindow&&) = delete;
  OsWindow& operator=(OsWindow&&) = delete;

  ~OsWindow();


  // Manually ask to quit
  void askToClose();

  // Might be triggered by the user pressing the "cross" button
  bool isBeingClosed();

  // Returns current resolution. Might be 0,0 if minimized
  glm::ivec2 getResolution();

  vk::UniqueSurfaceKHR createVkSurface(vk::Instance instance);

  GLFWwindow* native() const { return impl; }

  bool captureMouse = false;
  Keyboard keyboard;
  Mouse mouse;

private:
  OsWindowingManager* owner;
  GLFWwindow* impl;
  bool mouseWasCaptured = false;
};
