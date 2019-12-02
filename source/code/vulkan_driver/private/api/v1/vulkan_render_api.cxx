#include "vulkan_render_api.hxx"

#include <vulkan/vulkan.h>

namespace render::api::v1::vulkan
{

    static_assert(sizeof(VkCommandBuffer) == sizeof(command_buffer_handle), "Command buffer handle differs in size!");

    void vulkan_command_begin(command_buffer_handle cb, render_pass_handle rp, [[maybe_unused]] frame_buffer_handle fb, iRect rect) noexcept
    {
        VkRenderPassBeginInfo rp_begin{};
        rp_begin.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        rp_begin.pNext = nullptr;
        rp_begin.renderPass = reinterpret_cast<VkRenderPass>(rp);
        rp_begin.framebuffer = nullptr; /* info.framebuffers[frame_buffer_handle]; */

        rp_begin.renderArea.offset.x = rect.offset.x;
        rp_begin.renderArea.offset.y = rect.offset.y;
        rp_begin.renderArea.extent.width = rect.size.x;
        rp_begin.renderArea.extent.height = rect.size.y;

        VkClearValue clear_values[2];
        clear_values[0].color.float32[0] = 0.2f;
        clear_values[0].color.float32[1] = 0.2f;
        clear_values[0].color.float32[2] = 0.2f;
        clear_values[0].color.float32[3] = 0.2f;
        clear_values[1].depthStencil.depth = 1.0f;
        clear_values[1].depthStencil.stencil = 0;
        rp_begin.clearValueCount = 2;
        rp_begin.pClearValues = clear_values;

        vkCmdBeginRenderPass(reinterpret_cast<VkCommandBuffer>(cb), &rp_begin, VK_SUBPASS_CONTENTS_INLINE);
    }

    void vulkan_command_end(command_buffer_handle cb) noexcept
    {
        vkCmdEndRenderPass(reinterpret_cast<VkCommandBuffer>(cb));
    }

    void init_api(void* ptr) noexcept
    {
        auto instace = reinterpret_cast<render::api::v1::api_interface*>(ptr);
        instace->cmd_begin_func = vulkan_command_begin;
        instace->cmd_end_func = vulkan_command_end;
    }

} // namespace render::api::v1::vulkan
