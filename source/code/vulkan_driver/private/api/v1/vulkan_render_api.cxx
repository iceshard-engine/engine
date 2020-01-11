#include "vulkan_render_api.hxx"

#include <vulkan/vulkan.h>

namespace render::api::v1::vulkan
{

    auto native(CommandBuffer command_buffer) noexcept -> VkCommandBuffer
    {
        return reinterpret_cast<VkCommandBuffer>(command_buffer);
    }

    void vulkan_api_v1_initialized() noexcept
    {
        fmt::print("Using Vulkan Render API v1.\n");
    }

    void vulkan_command_begin(CommandBuffer cb) noexcept
    {
        VkCommandBufferBeginInfo cmd_buf_info = {};
        cmd_buf_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        cmd_buf_info.pNext = nullptr;
        cmd_buf_info.flags = 0;
        cmd_buf_info.pInheritanceInfo = nullptr;

        auto api_result = vkBeginCommandBuffer(native(cb), &cmd_buf_info);
        IS_ASSERT(api_result == VkResult::VK_SUCCESS, "Couldn't begin command buffer.");
    }

    void vulkan_renderpass_begin() noexcept
    {
        //VkRenderPassBeginInfo rp_begin{};
        //rp_begin.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        //rp_begin.pNext = nullptr;
        //rp_begin.renderPass = reinterpret_cast<VkRenderPass>(rp);
        //rp_begin.framebuffer = nullptr; /* info.framebuffers[frame_buffer_handle]; */

        //rp_begin.renderArea.offset.x = rect.offset.x;
        //rp_begin.renderArea.offset.y = rect.offset.y;
        //rp_begin.renderArea.extent.width = rect.size.x;
        //rp_begin.renderArea.extent.height = rect.size.y;

        //VkClearValue clear_values[2];
        //clear_values[0].color.float32[0] = 0.2f;
        //clear_values[0].color.float32[1] = 0.2f;
        //clear_values[0].color.float32[2] = 0.2f;
        //clear_values[0].color.float32[3] = 0.2f;
        //clear_values[1].depthStencil.depth = 1.0f;
        //clear_values[1].depthStencil.stencil = 0;
        //rp_begin.clearValueCount = 2;
        //rp_begin.pClearValues = clear_values;

        //vkCmdBeginRenderPass(reinterpret_cast<VkCommandBuffer>(cb), &rp_begin, VK_SUBPASS_CONTENTS_INLINE);
    }

    void vulkan_renderpass_end(CommandBuffer cb) noexcept
    {
        vkCmdEndRenderPass(native(cb));
    }

    void vulkan_command_end(CommandBuffer cb) noexcept
    {
        vkEndCommandBuffer(native(cb));
    }

    void init_api(void* ptr) noexcept
    {
        auto instace = reinterpret_cast<render::api::v1::RenderInterface*>(ptr);
        instace->check_func = vulkan_api_v1_initialized;
        instace->cmd_begin_func = vulkan_command_begin;
        instace->cmd_end_func = vulkan_command_end;
    }

} // namespace render::api::v1::vulkan
