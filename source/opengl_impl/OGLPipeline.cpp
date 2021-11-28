//
// Created by Бушев Дмитрий on 08.11.2021.
//

#include "OGLPipeline.h"
#include <stdexcept>

APITest::OGLGraphicsPipeline::OGLGraphicsPipeline(
    const APITest::GraphicsPipelineLayout &desc)
    : layout_(desc) {

  GLuint vertexShader = desc.vertexLayout.vertexShader;
  GLuint fragmentShader = desc.fragmentLayout.fragmentShader;

  if (!glIsShader(vertexShader) || !glIsShader(fragmentShader))
    throw std::runtime_error(
        "[OGL][ERROR]: Invalid shader id passed to pipeline description");

  program_ = glCreateProgram();

  glAttachShader(program_, vertexShader);
  glAttachShader(program_, fragmentShader);

  glLinkProgram(program_);

  if (glGetError() != GL_NO_ERROR)
    throw std::runtime_error("[OGL][ERROR]: failed to link shader program");

  glDetachShader(program_, vertexShader);
  glDetachShader(program_, fragmentShader);
}
static GLenum
convertToGLenum(APITest::RasterizerLayout::DepthTest::CompOp compOp) {
  switch (compOp) {
  case APITest::RasterizerLayout::DepthTest::CompOp::LESS_OR_EQUAL:
    return GL_LEQUAL;
  case APITest::RasterizerLayout::DepthTest::CompOp::LESS:
    return GL_LESS;
  case APITest::RasterizerLayout::DepthTest::CompOp::GREATER:
    return GL_GREATER;
  case APITest::RasterizerLayout::DepthTest::CompOp::GREATER_OR_EQUAL:
    return GL_GEQUAL;
  default:
    return GL_MAX;
  }
}
void APITest::OGLGraphicsPipeline::bind() const {
  glUseProgram(program_);

  if (layout().rasterizerLayout.cullingState.cullMode !=
      RasterizerLayout::CullingState::CULL_NONE) {
    glEnable(GL_CULL_FACE);

    if (layout().rasterizerLayout.cullingState.cullMode ==
        RasterizerLayout::CullingState::CULL_FRONT_FACE)
      glCullFace(GL_FRONT_FACE);
    else
      glCullFace(GL_BACK);

    if (layout().rasterizerLayout.cullingState.face ==
        RasterizerLayout::CullingState::FRONT_FACE_CCW)
      glFrontFace(GL_CCW);
    else
      glFrontFace(GL_CW);
  } else {
    glDisable(GL_CULL_FACE);
  }

  if (layout().blendingState.enable) {
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  } else {
    glDisable(GL_BLEND);
  }

  if (layout().rasterizerLayout.depthTest.enable) {
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(convertToGLenum(layout().rasterizerLayout.depthTest.compOp));
    // TODO something with write enabling (???)
  } else {
    glDisable(GL_DEPTH_TEST);
  }
}

APITest::OGLGraphicsPipeline::~OGLGraphicsPipeline() {
  glDeleteProgram(program_);
}

APITest::GraphicsPipelineRef
APITest::OGLPipelineManager::get(GraphicsPipelineLayout const &desc) {
  return std::unique_ptr<GraphicsPipeline>(new OGLGraphicsPipeline(desc));
}