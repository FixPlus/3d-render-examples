/**
 *   --------------------------- Model ------------------------------------
 *
 *   This example demonstrates loading and rendering basic GLTF model
 *
 */

#include "common.h"




EXAMPLE_MAIN_HEADER(Model)

    Examples::GLTFModel::Init(renderer);

    Camera camera(!APITest::isVulkan(renderer.get()));
    camera.allocateCameraBuffer(renderer);

    Examples::GLTFModel model{"model.gltf", camera};

    auto modelInstance = model.createNewInstance();

    auto renderPass = renderer->createOnscreenColorPass();
    renderPass->enableDepthBuffer(1);

    renderPass->commands = [&modelInstance](APITest::CommandRecorder* recorder){
        modelInstance.draw(recorder);
    };

    auto gui = renderer->getGUI();

    gui->onKeyDown = [&window, &camera, &example_close, &gui](int key){
        switch(key) {
            case APITest::KEY_C: window->cursorEnabled() ? window->disableCursor() : window->enableCursor(); break;
            case APITest::KEY_ESCAPE: example_close = true; break;
            case APITest::KEY_O: gui->toggle(); break;
            default: ;
        }

        camera.keyDown(key);
    };

    gui->onKeyUp = [&camera](int key){
        camera.keyUp(key);
    };

    gui->onMouseMove = [&window, &camera](double dx, double dy){
        if(!window->cursorEnabled()) {
            camera.mouseMove(dx, dy);
        }
    };

    window->onFramebufferResize = [&camera](int width, int height){
        if(height != 0) {
            camera.updateAspectRatio(static_cast<float>(width) / static_cast<float>(height));
        }
    };

    renderer->pushNewRenderPassGraph(renderPass);

MAIN_LOOP_HEAD()
{
    camera.update(renderer->statistics().lastFrameTime);
}
MAIN_LOOP_TAIL()
    return 0;
}