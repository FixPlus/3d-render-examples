//
// Created by Бушев Дмитрий on 14.11.2021.
//

#ifndef RENDERAPITEST_OGLTEXTURE_H
#define RENDERAPITEST_OGLTEXTURE_H

#include "RenderInterface.h"
#include <GL/glew.h>

namespace APITest {

class OGLTexture : public Image {
  GLuint texture_ = 0;
  ImageDesc imageDesc_;

public:
  explicit OGLTexture(ImageDesc desc);

  void load(void const *data) override;
  GLuint get() const { return texture_; }

  void bind();

  Type getType() const override { return imageDesc_.type; };
  Format getFormat() const override { return imageDesc_.format; };
  Extents getImageExtents() const override { return imageDesc_.extents; };
  uint32_t arrayLayers() const override { return imageDesc_.extents.layers; };
  Usage getUsage() const override { return imageDesc_.usage; };
};

} // namespace APITest
#endif // RENDERAPITEST_OGLTEXTURE_H
