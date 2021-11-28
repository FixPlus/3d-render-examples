//
// Created by Бушев Дмитрий on 27.10.2021.
//

#include "GLFWWindowImpl.h"
#include <stdexcept>
#include <string>

namespace APITest {
std::map<GLFWwindow *, GLFWWindowImpl *> GLFWWindowImpl::windowMap;

static int glfwKeyTranslate(int key) {
  switch (key) {
#define TRANSLATE_KEY(KEY)                                                     \
  case GLFW_KEY_##KEY:                                                         \
    return KEY_##KEY;
    TRANSLATE_KEY(A)
    TRANSLATE_KEY(B)
    TRANSLATE_KEY(C)
    TRANSLATE_KEY(D)
    TRANSLATE_KEY(E)
    TRANSLATE_KEY(F)
    TRANSLATE_KEY(G)
    TRANSLATE_KEY(H)
    TRANSLATE_KEY(I)
    TRANSLATE_KEY(J)
    TRANSLATE_KEY(K)
    TRANSLATE_KEY(L)
    TRANSLATE_KEY(M)
    TRANSLATE_KEY(N)
    TRANSLATE_KEY(O)
    TRANSLATE_KEY(P)
    TRANSLATE_KEY(Q)
    TRANSLATE_KEY(R)
    TRANSLATE_KEY(S)
    TRANSLATE_KEY(T)
    TRANSLATE_KEY(U)
    TRANSLATE_KEY(V)
    TRANSLATE_KEY(W)
    TRANSLATE_KEY(X)
    TRANSLATE_KEY(Y)
    TRANSLATE_KEY(Z)
    TRANSLATE_KEY(SEMICOLON)
    TRANSLATE_KEY(SLASH)
    TRANSLATE_KEY(COMMA)
    TRANSLATE_KEY(BACKSLASH)
    TRANSLATE_KEY(APOSTROPHE)
    TRANSLATE_KEY(SPACE)
    TRANSLATE_KEY(BACKSPACE)
    TRANSLATE_KEY(ESCAPE)
  case GLFW_KEY_RIGHT_BRACKET:
    return KEY_R_BRACKET;
  case GLFW_KEY_LEFT_BRACKET:
    return KEY_L_BRACKET;
    TRANSLATE_KEY(F1)
    TRANSLATE_KEY(F2)
    TRANSLATE_KEY(F3)
    TRANSLATE_KEY(F4)
    TRANSLATE_KEY(F5)
    TRANSLATE_KEY(F6)
    TRANSLATE_KEY(F7)
    TRANSLATE_KEY(F8)
    TRANSLATE_KEY(F9)
    TRANSLATE_KEY(F10)
    TRANSLATE_KEY(F11)
    TRANSLATE_KEY(RIGHT)
    TRANSLATE_KEY(LEFT)
    TRANSLATE_KEY(UP)
    TRANSLATE_KEY(DOWN)
    TRANSLATE_KEY(HOME)
    TRANSLATE_KEY(END)
    TRANSLATE_KEY(INSERT)
    TRANSLATE_KEY(DELETE)
    TRANSLATE_KEY(ENTER)
    TRANSLATE_KEY(LEFT_SHIFT)
    TRANSLATE_KEY(RIGHT_SHIFT)
    TRANSLATE_KEY(LEFT_CONTROL)
    TRANSLATE_KEY(RIGHT_CONTROL)
    TRANSLATE_KEY(LEFT_ALT)
    TRANSLATE_KEY(RIGHT_ALT)
#undef TRANSLATE_KEY
#define TRANSLATE_KEY(KEY)                                                     \
  case GLFW_KEY_##KEY:                                                         \
    return KEY_NUM_##KEY;
    TRANSLATE_KEY(1)
    TRANSLATE_KEY(2)
    TRANSLATE_KEY(3)
    TRANSLATE_KEY(4)
    TRANSLATE_KEY(5)
    TRANSLATE_KEY(6)
    TRANSLATE_KEY(7)
    TRANSLATE_KEY(8)
    TRANSLATE_KEY(9)
#undef TRANSLATE_KEY
#define TRANSLATE_KEY(KEY)                                                     \
  case GLFW_KEY_KP_##KEY:                                                      \
    return KEY_NUM_PAD_##KEY;
    TRANSLATE_KEY(1)
    TRANSLATE_KEY(2)
    TRANSLATE_KEY(3)
    TRANSLATE_KEY(4)
    TRANSLATE_KEY(5)
    TRANSLATE_KEY(6)
    TRANSLATE_KEY(7)
    TRANSLATE_KEY(8)
    TRANSLATE_KEY(9)
    TRANSLATE_KEY(ADD)
    TRANSLATE_KEY(SUBTRACT)
    TRANSLATE_KEY(DIVIDE)
    TRANSLATE_KEY(MULTIPLY)
    TRANSLATE_KEY(ENTER)
    TRANSLATE_KEY(DECIMAL)
    TRANSLATE_KEY(EQUAL)
  default:
    return KEY_UNKNOWN;
  }
};
} // namespace APITest

APITest::GLFWWindowImpl::GLFWWindowImpl(const APITest::WindowDesc &desc)
    : WindowInterface(desc.width, desc.height, desc.title) {

  if (glfwInit() != GLFW_TRUE)
    throw std::runtime_error("Failed to initialize GLFW. glfwInit() returned" +
                             std::to_string(glfwGetError(nullptr)));

  // TODO: glfwSetErrorCallback(error_callback);

  glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

  instance_ =
      glfwCreateWindow(width_, height_, title_.c_str(), nullptr, nullptr);

  windowMap.emplace(instance_, this);

  // Framebuffer size can actually be different from the size in screen
  // coordinates (from which window is created from). Most time it's not the
  // case but it will be fatal if mess up.

  int width_in_pixels, height_in_pixels;

  glfwGetFramebufferSize(instance_, &width_in_pixels, &height_in_pixels);

  width_ = width_in_pixels;
  height_ = height_in_pixels;

  setCallbacks();
}

VkSurfaceKHR APITest::GLFWWindowImpl::getSurface(VkInstance vkInstance) {
  VkSurfaceKHR surface;

  if (auto err =
          (glfwCreateWindowSurface(vkInstance, instance_, nullptr, &surface) -
           VK_SUCCESS))
    throw std::runtime_error(
        "[GLFW ERROR] Could not create Vulkan surface. Error code: " +
        std::to_string(err + VK_SUCCESS));

  return surface;
}

bool APITest::GLFWWindowImpl::handleEvents() {

  sizeChanged = false;
  glfwPollEvents();

  return glfwWindowShouldClose(instance_);
}

void APITest::GLFWWindowImpl::windowSizeChanged(GLFWwindow *window, int width,
                                                int height) {
  auto windowImpl = windowMap.find(window)->second;

  windowImpl->width_ = width;
  windowImpl->height_ = height;

  windowImpl->sizeChanged = true;
  windowImpl->callback.sizeChanged(width, height);
  windowImpl->onFramebufferResize(width, height);
}

APITest::GLFWWindowImpl::~GLFWWindowImpl() {
  windowMap.erase(instance_);
  glfwDestroyWindow(instance_);

  if (windowMap.empty())
    glfwTerminate();
}

std::vector<const char *> APITest::GLFWWindowImpl::getVulkanExtensions() {
  uint32_t count = 0;
  auto ext = glfwGetRequiredInstanceExtensions(&count);

  std::vector<const char *> ret(count);
  for (int i = 0; i < count; i++)
    ret.at(i) = ext[i];

  return ret;
}

bool APITest::GLFWWindowImpl::minimized() const {
  return glfwGetWindowAttrib(instance_, GLFW_ICONIFIED);
}

void APITest::GLFWWindowImpl::syncGLContext() {
  // need to recreate window due to window hint

  windowMap.erase(instance_);
  glfwDestroyWindow(instance_);

  // TODO: glfwSetErrorCallback(error_callback);

  glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_API);

  instance_ =
      glfwCreateWindow(width_, height_, title_.c_str(), nullptr, nullptr);

  windowMap.emplace(instance_, this);

  // Framebuffer size can actually be different from the size in screen
  // coordinates (from which window is created from). Most time it's not the
  // case but it will be fatal if mess up.

  int width_in_pixels, height_in_pixels;

  glfwGetFramebufferSize(instance_, &width_in_pixels, &height_in_pixels);

  width_ = width_in_pixels;
  height_ = height_in_pixels;

  glfwSetFramebufferSizeCallback(instance_, windowSizeChanged);
  glfwSetErrorCallback(errorHandle);

  glfwMakeContextCurrent(instance_);

  glfwSwapInterval(0);

  setCallbacks();
}

void APITest::GLFWWindowImpl::errorHandle(int error, const char *errorString) {
  throw std::runtime_error(
      "[GLFW][ERROR] GLFW library encountered following error: " +
      std::string(errorString) + "\nError code: " + std::to_string(error));
}
void APITest::GLFWWindowImpl::swapBuffers() { glfwSwapBuffers(instance_); }

void APITest::GLFWWindowImpl::resize(uint32_t width, uint32_t height) {
  glfwSetWindowSize(instance_, width, height);
  width_ = width;
  height_ = height;
}

bool APITest::GLFWWindowImpl::cursorEnabled() const { return cursorEnabled_; }

void APITest::GLFWWindowImpl::disableCursor() {
  if (!cursorEnabled_)
    return;
  glfwSetInputMode(instance_, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
  cursorEnabled_ = false;
}

void APITest::GLFWWindowImpl::enableCursor() {
  if (cursorEnabled_)
    return;
  glfwSetInputMode(instance_, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
  cursorEnabled_ = true;
}

void APITest::GLFWWindowImpl::keyInput(GLFWwindow *window, int key,
                                       int scancode, int action, int mods) {
  auto &wrappedWindow = *windowMap.at(window);

  key = glfwKeyTranslate(key);

  switch (action) {
  case GLFW_PRESS: {
    wrappedWindow.callback.keyDown(key);
    break;
  }
  case GLFW_RELEASE: {
    wrappedWindow.callback.keyUp(key);
    break;
  }
  case GLFW_REPEAT: {
    wrappedWindow.callback.keyPressed(key);
    break;
  }
  default:
    break;
  }
}

void APITest::GLFWWindowImpl::cursorPosition(GLFWwindow *window, double xPos,
                                             double yPos) {
  windowMap.at(window)->callback.mouseMoved(xPos, yPos);
}

void APITest::GLFWWindowImpl::setCallbacks() {
  glfwSetFramebufferSizeCallback(instance_, windowSizeChanged);
  glfwSetKeyCallback(instance_, keyInput);
  glfwSetCursorPosCallback(instance_, cursorPosition);
  glfwSetMouseButtonCallback(instance_, mouseInput);
  glfwSetScrollCallback(instance_, scrollInput);
  glfwSetCharCallback(instance_, charInput);
  glfwSetErrorCallback(errorHandle);
}

void APITest::GLFWWindowImpl::mouseInput(GLFWwindow *window, int button,
                                         int action, int mods) {
  auto &wrappedWindow = windowMap.at(window);
  if (action == GLFW_PRESS) {
    wrappedWindow->callback.mouseBtnDown(button);
  } else {
    wrappedWindow->callback.mouseBtnUp(button);
  }
}

void APITest::GLFWWindowImpl::scrollInput(GLFWwindow *window, double xoffset,
                                          double yoffset) {
  windowMap.at(window)->callback.mouseScroll(xoffset, yoffset);
}

void APITest::GLFWWindowImpl::charInput(GLFWwindow *window, uint32_t unicode) {
  windowMap.at(window)->callback.charInput(unicode);
}
