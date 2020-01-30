#pragma once
#include <render_system/render_api.hxx>
#include <iceshard/renderer/vulkan/vulkan_sdk.hxx>

namespace render::api::v1::vulkan
{

    void init_api(void* ptr) noexcept;

    struct RenderPassContext
    {
        VkRenderPass renderpass;
        VkFramebuffer framebuffer;
        VkExtent2D extent;
    };

    struct CommandBufferContext
    {
        VkCommandBuffer command_buffer;
        RenderPassContext* render_pass_context;
    };

    auto native(CommandBuffer command_buffer) noexcept -> CommandBufferContext const*;

} // namespace render::api::v1::vulkan
