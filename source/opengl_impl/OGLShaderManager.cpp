//
// Created by Бушев Дмитрий on 08.11.2021.
//

#include "OGLShaderManager.h"
#include <fstream>

APITest::OGLShader::OGLShader(std::string source, GLenum type)
    : source_(source), type_(type) {
  shader_ = glCreateShader(type);
  auto *code = source_.data();
  GLint length = source_.size();
  glShaderSource(shader_, 1, &code, &length);
  glCompileShader(shader_);
  GLint status;
  glGetShaderiv(shader_, GL_COMPILE_STATUS, &status);
  if (glGetError() != GL_NO_ERROR || status == GL_FALSE) {
    std::string log;
    GLsizei logSize;
    glGetShaderiv(shader_, GL_INFO_LOG_LENGTH, &logSize);
    log.resize(logSize);
    glGetShaderInfoLog(shader_, logSize, &logSize, log.data());

    throw std::runtime_error(
        "[OGL][ERROR]: failed to compile shader program. Shader log:\n" + log);
  }
}

APITest::OGLShader::OGLShader(APITest::OGLShader &&another) {
  shader_ = another.shader_;
  source_ = another.source_;
  type_ = another.type_;

  another.shader_ = 0;
}

APITest::OGLShader const &
APITest::OGLShader::operator=(APITest::OGLShader &&another) {
  shader_ = another.shader_;
  source_ = another.source_;
  type_ = another.type_;

  another.shader_ = 0;
  return *this;
}

APITest::OGLShader::~OGLShader() { glDeleteShader(shader_); }

GLuint APITest::OGLShaderManager::loadShader(std::string filename,
                                             GLenum stage) {
  std::ifstream is(filename, std::ios::binary | std::ios::in | std::ios::ate);

  if (is.is_open()) {
    size_t size = is.tellg();
    is.seekg(0, std::ios::beg);
    char *shaderCode = new char[size + 1];
    is.read(shaderCode, size);
    is.close();

    if (size == 0)
      throw std::runtime_error(
          "[OGL][ERROR]: shader module load failed: file is empty. Filename: " +
          filename);

    shaderCode[size] = '\0';

    OGLShader shader(std::string(shaderCode), stage);

    delete[] shaderCode;

    auto ret = shader.get();

    shaderMap.emplace(std::piecewise_construct, std::forward_as_tuple(ret),
                      std::forward_as_tuple(std::move(shader)));
    return ret;
  } else {
    throw std::runtime_error("[OGL][ERROR]: shader module load failed: could "
                             "not open file. Filename: " +
                             filename);
  }
}
