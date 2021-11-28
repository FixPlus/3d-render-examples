//
// Created by Бушев Дмитрий on 08.11.2021.
//

#ifndef RENDERAPITEST_OGLFRAMEBUFFER_H
#define RENDERAPITEST_OGLFRAMEBUFFER_H

#include <GL/glew.h>

namespace APITest {

class OGLFramebuffer {
  bool onscreen = true;
  GLuint buffer_ = 0;

public:
  OGLFramebuffer() = default;
  void bind() const;
};
} // namespace APITest
#endif // RENDERAPITEST_OGLFRAMEBUFFER_H
