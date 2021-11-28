//
// Created by Бушев Дмитрий on 08.11.2021.
//

#ifndef RENDERAPITEST_OGLPIPELINE_H
#define RENDERAPITEST_OGLPIPELINE_H

#include "RenderInterface.h"
#include <GL/glew.h>
#include <map>

namespace APITest {

class OGLPipeline : virtual public Pipeline {
protected:
  GLuint program_;

public:
  virtual void bind() const = 0;
};

class OGLGraphicsPipeline : public OGLPipeline, public GraphicsPipeline {
  GraphicsPipelineLayout layout_;

public:
  OGLGraphicsPipeline(GraphicsPipelineLayout const &desc);

  OGLGraphicsPipeline(OGLGraphicsPipeline &&another) {
    program_ = another.program_;
    layout_ = std::move(another.layout_);
    another.program_ = 0;
  }

  OGLGraphicsPipeline &operator=(OGLGraphicsPipeline &&another) noexcept {
    program_ = another.program_;
    layout_ = std::move(another.layout_);
    another.program_ = 0;
    return *this;
  }

  GraphicsPipelineLayout const &layout() const override { return layout_; }

  void bind() const override;

  ~OGLGraphicsPipeline() override;
};

struct OGLPipelineManager {
  GraphicsPipelineRef get(GraphicsPipelineLayout const &desc);
};
} // namespace APITest
#endif // RENDERAPITEST_OGLPIPELINE_H
