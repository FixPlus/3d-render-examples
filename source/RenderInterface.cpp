//
// Created by Бушев Дмитрий on 27.10.2021.
//

#include "RenderInterface.h"
#include "GL/glew.h" // glew must be included before GLFWWindowImpl.h
#include "glfw_window_impl/GLFWWindowImpl.h"
#include "imgui_impl/IGGuiImpl.h"
#include "opengl_impl/OGLRenderImpl.h"
#include "vulkan_impl/VulkanRenderImpl.h"

namespace APITest {

WindowRef createGLFWWindow(const WindowDesc &desc) {
  return WindowRef(new GLFWWindowImpl(desc));
}

RenderInterfaceRef createVulkanAPI(const WindowRef &window,
                                   bool enableValidation) {
  return RenderInterfaceRef(new VulkanRenderImpl(window, enableValidation));
}

RenderInterfaceRef createOpenGLAPI(WindowRef window) {
  return RenderInterfaceRef(new OGLRenderImpl(std::move(window)));
}

GUIRef createImGui(RenderInterface *renderInterface) {
  return GUIRef(new IGGuiImpl(renderInterface));
}

bool isVulkan(RenderInterface const *api) {
  return dynamic_cast<VulkanRenderImpl const *>(api);
}
bool isOpenGL(RenderInterface const *api) {
  return dynamic_cast<OGLRenderImpl const *>(api);
}

} // namespace APITest