//
// Created by Бушев Дмитрий on 27.10.2021.
//

#ifndef RENDERAPITEST_WINDOWINTERFACE_H
#define RENDERAPITEST_WINDOWINTERFACE_H

#include "RenderInterface.h"
#include "vulkan/vulkan.h"
#include <functional>
#include <memory>
#include <utility>

namespace APITest {

/**
 *
 * This struct is for internal use only. This header is NOT a part of Render
 * interface.
 *
 * */

class WindowInterface : public Window {
protected:
  std::string title_;

public:
  uint32_t width_, height_;

  bool sizeChanged = false;

  explicit WindowInterface(uint32_t width = 800, uint32_t height = 600,
                           std::string title = "APITest")
      : width_(width), height_(height), title_(std::move(title)) {}

  /** Surface extractor for Vulkan API. */
  virtual VkSurfaceKHR getSurface(VkInstance vkInstance) = 0;

  /** OpenGL use only */
  virtual void syncGLContext() = 0;
  virtual void swapBuffers() = 0;

  uint32_t width() const override { return width_; };
  uint32_t height() const override { return height_; };

  /** Get required vulkan extensions to run. */
  virtual std::vector<const char *> getVulkanExtensions() = 0;

  /** Event callback interface. */
  struct {
    std::function<void(uint32_t, uint32_t)> sizeChanged = [](uint32_t,
                                                             uint32_t) {};
    std::function<void(int)> keyDown = [](int) {};
    std::function<void(int)> keyPressed = [](int) {};
    std::function<void(int)> keyUp = [](int) {};
    std::function<void(double, double)> mouseMoved = [](double, double) {};
    std::function<void(int)> mouseBtnDown = [](int) {};
    std::function<void(int)> mouseBtnUp = [](int) {};
    std::function<void(double, double)> mouseScroll = [](double, double) {};
    std::function<void(uint32_t)> charInput = [](uint32_t) {};
  } callback;

  virtual ~WindowInterface() = default;
};

} // namespace APITest
#endif // RENDERAPITEST_WINDOWINTERFACE_H
