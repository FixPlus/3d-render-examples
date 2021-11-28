//
// Created by Бушев Дмитрий on 27.10.2021.
//

#ifndef RENDERAPITEST_GLFWWINDOWIMPL_H
#define RENDERAPITEST_GLFWWINDOWIMPL_H

#include "RenderInterface.h"
#include "WindowInterface.h"
#include <GLFW/glfw3.h>
#include <map>

namespace APITest {

class GLFWWindowImpl final : public WindowInterface {
  static std::map<GLFWwindow *, GLFWWindowImpl *> windowMap;
  bool cursorEnabled_ = true;

  // callbacks

  static void windowSizeChanged(GLFWwindow *window, int width, int height);
  static void keyInput(GLFWwindow *window, int key, int scancode, int action,
                       int mods);
  static void cursorPosition(GLFWwindow *window, double xPos, double yPos);
  static void mouseInput(GLFWwindow *window, int button, int action, int mods);
  static void scrollInput(GLFWwindow *window, double xoffset, double yoffset);
  static void charInput(GLFWwindow *window, uint32_t unicode);
  static void errorHandle(int error, const char *errorString);

  GLFWwindow *instance_ = nullptr;

  void setCallbacks();

public:
  explicit GLFWWindowImpl(const WindowDesc &desc);

  /** Surface extractor for Vulkan API. */
  VkSurfaceKHR getSurface(VkInstance vkInstance) override;

  void syncGLContext() override;
  void swapBuffers() override;

  bool handleEvents() override;

  bool minimized() const override;

  bool cursorEnabled() const override;
  void disableCursor() override;
  void enableCursor() override;

  std::vector<const char *> getVulkanExtensions() override;

  void resize(uint32_t width, uint32_t height) override;

  ~GLFWWindowImpl() override;
};
} // namespace APITest
#endif // RENDERAPITEST_GLFWWINDOWIMPL_H
