#ifndef CBVH_STF_GLFW_WINDOW_H
#define CBVH_STF_GLFW_WINDOW_H

#include "GLFW/glfw3.h"
#include "../render/render_common.h"


void onKeyboardPressedBasic(GLFWwindow* window, int key, int scancode, int action, int mode);
void onMouseButtonClickedBasic(GLFWwindow* window, int button, int action, int mods);
void onMouseMoveBasic(GLFWwindow* window, double xpos, double ypos);
void onMouseScrollBasic(GLFWwindow* window, double xoffset, double yoffset);

GLFWwindow * initWindow(int width, int height,
                        GLFWkeyfun keyboard = onKeyboardPressedBasic,
                        GLFWcursorposfun mouseMove = onMouseMoveBasic,
                        GLFWmousebuttonfun mouseBtn = onMouseButtonClickedBasic,
                        GLFWscrollfun mouseScroll = onMouseScrollBasic);

void mainLoop(IRender &app, GLFWwindow* window, bool displayGUI = false);

void setupImGuiContext(GLFWwindow* a_window);

#endif //CBVH_STF_GLFW_WINDOW_H
