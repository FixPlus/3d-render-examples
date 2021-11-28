//
// Created by Бушев Дмитрий on 10.11.2021.
//

#ifndef RENDERAPITEST_OGLDEBUG_H
#define RENDERAPITEST_OGLDEBUG_H

#include <GL/glew.h>
#include <stdexcept>

namespace APITest {

inline std::string errorString(GLenum errorCode) {
  switch (errorCode) {
#define STR(r)                                                                 \
  case GL_##r:                                                                 \
    return #r
    STR(INVALID_ENUM);
    STR(INVALID_VALUE);
    STR(INVALID_OPERATION);
    STR(INVALID_FRAMEBUFFER_OPERATION);
    STR(OUT_OF_MEMORY);
    STR(STACK_UNDERFLOW);
    STR(STACK_OVERFLOW);
#undef STR
  default:
    return "UNKNOWN_ERROR";
  }
}

inline void checkGLError() {
  if (auto error = glGetError())
    throw std::runtime_error("[OGL][ERROR] OpenGL call returned " +
                             errorString(error));
}
} // namespace APITest
#endif // RENDERAPITEST_OGLDEBUG_H
