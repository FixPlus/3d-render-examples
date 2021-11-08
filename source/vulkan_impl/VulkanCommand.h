//
// Created by Бушев Дмитрий on 07.11.2021.
//

#ifndef RENDERAPITEST_VULKANCOMMAND_H
#define RENDERAPITEST_VULKANCOMMAND_H

#include <vulkan/vulkan.h>

namespace APITest{


    struct VulkanCommand{
        virtual void record(VkCommandBuffer buffer) const = 0;
        void execute(VkQueue queue, VkDevice device);
    };


    struct VulkanDrawCall: public VulkanCommand{
        VkPipeline pipeline;

        explicit VulkanDrawCall(VkPipeline pipe): pipeline(pipe){}
        void record(VkCommandBuffer buffer) const override;
    };
}
#endif //RENDERAPITEST_VULKANCOMMAND_H
