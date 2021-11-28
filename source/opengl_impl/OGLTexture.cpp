//
// Created by Бушев Дмитрий on 14.11.2021.
//

#include "OGLTexture.h"
#include "OGLDebug.h"
#include <cassert>
#include <stdexcept>

static GLenum convertInterfaceTypeToGLTarget(APITest::Image::Type type,
                                             uint32_t arrayLayers) {
  switch (type) {
  case APITest::Image::Type::TEXTURE_1D:
    return arrayLayers > 1 ? GL_TEXTURE_1D_ARRAY : GL_TEXTURE_1D;
  case APITest::Image::Type::TEXTURE_2D:
    return arrayLayers > 1 ? GL_TEXTURE_2D_ARRAY : GL_TEXTURE_2D;
  case APITest::Image::Type::TEXTURE_3D:
    if (arrayLayers > 1)
      throw std::runtime_error(
          "[OGL][ERROR]: arrayed 3D textures are not supported");
    return GL_TEXTURE_3D;
  default:
    throw std::runtime_error("[OGL][ERROR]: invalid texture format");
  }
}
static GLenum convertInterfaceImageFormat(APITest::Image::Format format) {
  switch (format) {
  case APITest::Image::Format::RGBA8:
    return GL_RGBA;
  case APITest::Image::Format::D24S8:
    return GL_DEPTH_STENCIL;
  case APITest::Image::Format::R11G11B10:
    return GL_R11F_G11F_B10F;
  default:
    throw std::runtime_error(
        "[OGL][ERROR]: could not create image: invalid pixel format");
  }
}
APITest::OGLTexture::OGLTexture(APITest::ImageDesc desc) : imageDesc_(desc) {
  glGenTextures(1, &texture_);

  GLenum target =
      convertInterfaceTypeToGLTarget(desc.type, desc.extents.layers);

  glBindTexture(target, texture_);
}

void APITest::OGLTexture::load(const void *data) {

  bind();
  auto target = convertInterfaceTypeToGLTarget(imageDesc_.type,
                                               imageDesc_.extents.layers);
  if (imageDesc_.type == Image::Type::TEXTURE_2D) {
    auto internalFormat = convertInterfaceImageFormat(imageDesc_.format);
    auto dataFormat =
        internalFormat == GL_R11F_G11F_B10F ? GL_RGB : internalFormat;
    glTexImage2D(target, 0, internalFormat, imageDesc_.extents.width,
                 imageDesc_.extents.height, 0, dataFormat, GL_UNSIGNED_BYTE,
                 data);
    checkGLError();
  } else {
    assert(0 && "not implemented yet.");
  }
}

void APITest::OGLTexture::bind() {
  GLenum target = convertInterfaceTypeToGLTarget(imageDesc_.type,
                                                 imageDesc_.extents.layers);
  glBindTexture(target, texture_);
}
