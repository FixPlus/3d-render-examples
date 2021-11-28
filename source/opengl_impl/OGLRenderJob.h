//
// Created by Бушев Дмитрий on 08.11.2021.
//

#ifndef RENDERAPITEST_OGLRENDERJOB_H
#define RENDERAPITEST_OGLRENDERJOB_H

#include "RenderInterface.h"
#include "opengl_impl/OGLFrameBuffer.h"
#include "opengl_impl/OGLPipeline.h"

namespace APITest {

class OGLRenderImpl;

class OGLRenderPass : virtual public RenderPass {
public:
  virtual void enable() = 0;
};
class OGLColorPass : virtual public OGLRenderPass, virtual public ColorPass {
protected:
  Image::Extents extents;

public:
  Image::Extents getFramebufferExtents() const override { return extents; };
};

class OGLOnscreenRenderPass final : public OGLColorPass,
                                    public OnscreenRenderPass {

public:
  void resetWindowFrameBufferExtents(uint32_t width, uint32_t height) {
    extents.width = width;
    extents.height = height;
  };
  void setColorBufferIndex(uint32_t binding) override{/* TODO */};
  void enableDepthBuffer(uint32_t binding) override{/* TODO */};
  void enable() override;
};

class OGLRenderJob {
  OGLRenderImpl *parent_;
  struct RenderUnit {
    RenderPassRef renderPass;
  };

  std::vector<RenderUnit> renderSequence;

  void compilePass(RenderPassRef node);

public:
  OGLRenderJob(OGLRenderImpl *parent, RenderPassRef node);

  void complete() const;
};

} // namespace APITest
#endif // RENDERAPITEST_OGLRENDERJOB_H
