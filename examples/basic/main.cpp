#include "common.h"

/**
 * ----------------------------------------- basic/main.cpp
 * ---------------------------------------------------------
 *
 * This example demonstrates the basics of usage this wrapper library and APIs
 * themself.
 *
 * */

EXAMPLE_MAIN_HEADER(Basic)

// Set arbitrary window surface size in pixels.
window->resize(800, 800);

// GraphicsPipelineLayout - description of GPU 3D graphics pipeline state.
APITest::GraphicsPipelineLayout pipelineLayout{};

// Shaders - programmable stages of 3D graphics pipeline. They are executed on
// GPU on multiple parallel cores.
pipelineLayout.vertexLayout.vertexShader = renderer->createShaderProgram(
    {use_vulkan ? ROOT_SHADERS_DIR "basic/simple.vert.spv"
                : ROOT_SHADERS_DIR "basic/simple.vert",
     APITest::ShaderStage::VERTEX});
pipelineLayout.fragmentLayout.fragmentShader = renderer->createShaderProgram(
    {use_vulkan ? ROOT_SHADERS_DIR "basic/simple.frag.spv"
                : ROOT_SHADERS_DIR "basic/simple.frag",
     APITest::ShaderStage::FRAGMENT});

// createGraphicsPipeline() - retrieves handle for a API-defined pipeline
// object.
auto pipeline = renderer -> createGraphicsPipeline(pipelineLayout);

// createOnscreenColorPass() - retrieves handle for a RenderPass object - a
// description of draw commands and target image we are drawing into. This
// particular function gives RenderPass that describes window surface rendering.
auto renderPass = renderer -> createOnscreenColorPass();

// Commands that would be executed every frame inside renderPass
renderPass->commands = [&pipeline](APITest::CommandRecorder *recorder) {
  recorder->bindPipeline(pipeline.get());
  recorder->draw(30, 0);
};

// Tell wrapper to use this set of commands for further rendering
renderer->pushNewRenderPassGraph(renderPass);

// Instantiates GUI to show frame statistics
renderer->getGUI();

MAIN_LOOP_HEAD() {}
MAIN_LOOP_TAIL()

return 0;
}
