//
// Created by Бушев Дмитрий on 13.11.2021.
//

#ifndef RENDERAPITEST_GUIINTERFACE_H
#define RENDERAPITEST_GUIINTERFACE_H

#include "RenderInterface.h"
#include "WindowInterface.h"
#include <stdexcept>

namespace APITest {

struct CommandRecorder;

class GuiInterface : public GUI {
protected:
  RenderInterface *renderInterface_ = nullptr;
  WindowInterface *windowInterface_ = nullptr;

public:
  explicit GuiInterface(RenderInterface *renderInterface)
      : renderInterface_(renderInterface),
        windowInterface_(dynamic_cast<WindowInterface *>(
            renderInterface->getWindow().get())) {
    if (!windowInterface_)
      throw std::runtime_error("[GUI][ERROR]: could not create GUI if no "
                               "window attached to renderer.");
  }

  /** Updates the state of UI: reads specific window data, collecting setting
   * user input information, etc.*/
  virtual void update() = 0;

  /** Prepares User Interface Overlay contents for a current frame. */
  virtual void render() = 0;

  /** Draws Overlay. */
  virtual void draw(CommandRecorder *commandRecorder) = 0;

  /** this callbacks are called from RenderInterface callback processing
   * functions. */
  /** Full hierarchy: WindowCallback -> RenderCallback -> GUICallback -> User
   * Defined callback. */

  virtual void keyDown(int key) = 0;
  virtual void keyPressed(int key) = 0;
  virtual void keyUp(int key) = 0;
  virtual void mouseMoved(double xPos, double yPos) = 0;
  virtual void mouseBtnDown(int button) = 0;
  virtual void mouseBtnUp(int button) = 0;
  virtual void mouseScroll(double xoffset, double yoffset) = 0;
  virtual void charInput(uint32_t unicode) = 0;
};

GUIRef createImGui(RenderInterface *renderInterface);
} // namespace APITest
#endif // RENDERAPITEST_GUIINTERFACE_H
