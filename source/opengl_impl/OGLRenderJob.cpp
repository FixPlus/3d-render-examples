//
// Created by Бушев Дмитрий on 08.11.2021.
//

#include "OGLRenderJob.h"
#include "GuiInterface.h"
#include "OGLCommandRecorder.h"
#include "opengl_impl/OGLRenderImpl.h"
#include <cassert>

void APITest::OGLRenderJob::complete() const {

  OGLCommandRecorder recorder;
  auto &statRecorder = parent_->recorder();

  statRecorder.push("Command record");

  for (auto &unit : renderSequence) {
    auto pass = unit.renderPass.lock();
    statRecorder.stamp(
        "Render pass " +
        std::to_string(reinterpret_cast<unsigned long long>(pass.get())));
    auto *oglPass = dynamic_cast<OGLRenderPass *>(pass.get());
    oglPass->enable();
    oglPass->commands(&recorder);
    if (parent_->getGuiInterface() &&
        dynamic_cast<OnscreenRenderPass *>(oglPass)) {
      parent_->getGuiInterface()->draw(&recorder);
    }
  }

  statRecorder.pop();
}

void APITest::OGLRenderJob::compilePass(RenderPassRef node) {
  auto pass = node.lock();

  for (auto &dep : pass->dependencies)
    compilePass(dep);

  RenderUnit unit;
  unit.renderPass = node;

  renderSequence.emplace_back(std::move(unit));
}

APITest::OGLRenderJob::OGLRenderJob(OGLRenderImpl *parent, RenderPassRef node)
    : parent_(parent) {
  compilePass(node);
}

void APITest::OGLOnscreenRenderPass::enable() {
  // TODO: actual framebuffer enabling here

  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}
