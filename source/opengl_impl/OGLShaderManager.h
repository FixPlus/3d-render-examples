//
// Created by Бушев Дмитрий on 08.11.2021.
//

#ifndef RENDERAPITEST_OGLSHADERMANAGER_H
#define RENDERAPITEST_OGLSHADERMANAGER_H

#include "GL/glew.h"
#include <map>
#include <string>

namespace APITest {

class OGLShader {
  GLuint shader_;
  GLenum type_;
  std::string source_;

public:
  GLuint get() const { return shader_; }
  GLenum getType() const { return type_; }

  OGLShader(std::string source, GLenum type);

  OGLShader(OGLShader &&another);

  OGLShader const &operator=(OGLShader &&another);

  ~OGLShader();
};

class OGLShaderManager {
  std::map<GLuint, OGLShader> shaderMap;

public:
  GLuint loadShader(std::string filename, GLenum stage);
  OGLShader const &get(GLuint shaderID) const {
    return shaderMap.at(shaderID);
  };
  OGLShader &get(GLuint shaderID) { return shaderMap.at(shaderID); };
};
} // namespace APITest
#endif // RENDERAPITEST_OGLSHADERMANAGER_H
