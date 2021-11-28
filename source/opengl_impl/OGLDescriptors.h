//
// Created by Бушев Дмитрий on 16.11.2021.
//

#ifndef RENDERAPITEST_OGLDESCRIPTORS_H
#define RENDERAPITEST_OGLDESCRIPTORS_H

#include "RenderInterface.h"

namespace APITest {

class OGLDescriptorSetLayout final : public DescriptorSetLayout {

public:
  explicit OGLDescriptorSetLayout(std::vector<DescriptorLayout> const &desc){};

  UniformDescriptorSetRef allocateNewSet(UniformDescriptor *descriptors,
                                         int count) override;
};

class OGLDescriptorSet final : public UniformDescriptorSet {
  std::vector<UniformDescriptor> descriptors;

public:
  OGLDescriptorSet(UniformDescriptor *descriptor, int count)
      : descriptors(count) {
    for (int i = 0; i < count; ++i)
      descriptors.at(i) = descriptor[i];
  };

  void bind() const;
};

} // namespace APITest
#endif // RENDERAPITEST_OGLDESCRIPTORS_H
