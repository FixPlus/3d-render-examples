//
// Created by Бушев Дмитрий on 01.11.2021.
//

#ifndef RENDERAPITEST_VULKANRENDERTARGET_H
#define RENDERAPITEST_VULKANRENDERTARGET_H

#include <vulkan/vulkan.h>
#include <tuple>

namespace APITest{


    /**
     *
     *  This class represents an abstraction of an image used as render target.
     *  It is needed to represent in common way allocated image on GPU and
     *  swap chain image that is implicitly created by swap chain
     *
     */
    class RenderTarget{
    public:
        VkImageView getTargetAttachmentView();
        VkFormat getFormat();
        std::pair<uint32_t, uint32_t> getExtents();
    };
}
#endif //RENDERAPITEST_VULKANRENDERTARGET_H
