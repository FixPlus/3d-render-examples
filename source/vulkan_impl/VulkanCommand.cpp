//
// Created by Бушев Дмитрий on 07.11.2021.
//

#include "VulkanCommand.h"
#include <cassert>

void APITest::VulkanCommand::execute(VkQueue queue, VkDevice device) {
    assert(0 && "Not implemented yet");
}

void APITest::VulkanDrawCall::record(VkCommandBuffer buffer) const{
    vkCmdBindPipeline(buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
    vkCmdDraw(buffer, 3, 1, 0, 0);
}
