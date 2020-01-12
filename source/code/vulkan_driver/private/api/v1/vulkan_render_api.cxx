#include "vulkan_render_api.hxx"
#include "../../vulkan_buffer.hxx"
#include "../../pipeline/vulkan_descriptor_sets.hxx"
#include <core/memory.hxx>

#include <vulkan/vulkan.h>

namespace render::api::v1::vulkan
{

    auto native(CommandBuffer command_buffer) noexcept -> render::api::v1::vulkan::CommandBufferContext const*
    {
        return reinterpret_cast<render::api::v1::vulkan::CommandBufferContext*>(command_buffer);
    }

    auto command_buffer(CommandBuffer command_buffer) noexcept -> VkCommandBuffer
    {
        return native(command_buffer)->command_buffer;
    }

    void vulkan_api_v1_initialized() noexcept
    {
        fmt::print("Using Vulkan Render API v1.\n");
    }

    void vulkan_api_v1_buffer_map(render::api::v1::VertexBuffer vertex_buffer, BufferDataView& buffer_data)
    {
        reinterpret_cast<render::vulkan::VulkanBuffer*>(vertex_buffer)->map_memory(buffer_data);
    }

    void vulkan_api_v1_buffer_unmap(render::api::v1::VertexBuffer vertex_buffer)
    {
        reinterpret_cast<render::vulkan::VulkanBuffer*>(vertex_buffer)->unmap_memory();
    }

    void vulkan_api_v1_uniform_buffer_map(render::api::v1::UniformBuffer uniform_buffer, BufferDataView& buffer_data)
    {
        reinterpret_cast<render::vulkan::VulkanBuffer*>(uniform_buffer)->map_memory(buffer_data);
    }

    void vulkan_api_v1_uniform_buffer_unmap(render::api::v1::UniformBuffer uniform_buffer)
    {
        reinterpret_cast<render::vulkan::VulkanBuffer*>(uniform_buffer)->unmap_memory();
    }

    void vulkan_api_v1_command_begin(render::api::v1::CommandBuffer cb) noexcept
    {
        VkCommandBufferBeginInfo cmd_buf_info = {};
        cmd_buf_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        cmd_buf_info.pNext = nullptr;
        cmd_buf_info.flags = 0;
        cmd_buf_info.pInheritanceInfo = nullptr;

        auto api_result = vkBeginCommandBuffer(command_buffer(cb), &cmd_buf_info);
        IS_ASSERT(api_result == VkResult::VK_SUCCESS, "Couldn't begin command buffer.");
    }

    void vulkan_api_v1_renderpass_begin(render::api::v1::CommandBuffer cb) noexcept
    {
        auto* command_buffer_context = native(cb);

        VkClearValue clear_values[2];
        clear_values[0].color.float32[0] = 0.2f;
        clear_values[0].color.float32[1] = 0.2f;
        clear_values[0].color.float32[2] = 0.2f;
        clear_values[0].color.float32[3] = 0.2f;
        clear_values[1].depthStencil.depth = 1.0f;
        clear_values[1].depthStencil.stencil = 0;

        VkRenderPassBeginInfo rp_begin;
        rp_begin.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        rp_begin.pNext = NULL;
        rp_begin.renderPass = command_buffer_context->render_pass_context->renderpass;
        rp_begin.framebuffer = command_buffer_context->render_pass_context->framebuffer;
        rp_begin.renderArea.offset.x = 0;
        rp_begin.renderArea.offset.y = 0;
        rp_begin.renderArea.extent = command_buffer_context->render_pass_context->extent;
        rp_begin.clearValueCount = 2;
        rp_begin.pClearValues = clear_values;

        vkCmdBeginRenderPass(command_buffer_context->command_buffer, &rp_begin, VK_SUBPASS_CONTENTS_INLINE);
    }

    void vulkan_api_v1_bind_render_pipeline(render::api::v1::CommandBuffer cb, render::api::v1::RenderPipeline pipeline) noexcept
    {
        vkCmdBindPipeline(command_buffer(cb), VK_PIPELINE_BIND_POINT_GRAPHICS, reinterpret_cast<VkPipeline>(pipeline));
    }

    void vulkan_api_v1_bind_descriptor_sets(render::api::v1::CommandBuffer cb, render::api::v1::DescriptorSets descriptor_sets) noexcept
    {
        auto* ctx = native(cb);

        const auto* desc_sets = reinterpret_cast<render::vulkan::VulkanDescriptorSets const*>(descriptor_sets);
        const auto& sets = desc_sets->native_handles();

        vkCmdBindDescriptorSets(
            ctx->command_buffer,
            VK_PIPELINE_BIND_POINT_GRAPHICS,
            ctx->render_pass_context->pipeline_layout,
            0,
            core::pod::array::size(sets),
            core::pod::array::begin(sets),
            0,
            NULL
        );
    }

    void vulkan_api_v1_bind_vertex_buffers(render::api::v1::CommandBuffer cb, render::api::v1::VertexBuffer vertice, render::api::v1::VertexBuffer instance) noexcept
    {
        VkDeviceSize const offsets[2] = { 0, 0 };
        VkBuffer const buffers[2] = {
            reinterpret_cast<render::vulkan::VulkanBuffer*>(vertice)->native_handle(),
            reinterpret_cast<render::vulkan::VulkanBuffer*>(instance)->native_handle()
        };
        vkCmdBindVertexBuffers(command_buffer(cb), 0, 2, buffers, offsets);
    }

    void vulkan_api_v1_set_viewport(render::api::v1::CommandBuffer cb, uint32_t width, uint32_t height) noexcept
    {
        VkViewport viewport{};
        viewport.width = (float)width;
        viewport.height = (float)height;
        viewport.minDepth = (float)0.0f;
        viewport.maxDepth = (float)1.0f;
        viewport.x = 0;
        viewport.y = 0;
        vkCmdSetViewport(command_buffer(cb), 0, 1, &viewport);
    }

    void vulkan_api_v1_set_scissor(render::api::v1::CommandBuffer cb, uint32_t width, uint32_t height) noexcept
    {
        VkRect2D scissor{};
        scissor.extent = VkExtent2D{ width, height };
        scissor.offset.x = 0;
        scissor.offset.y = 0;
        vkCmdSetScissor(command_buffer(cb), 0, 1, &scissor);
    }

    void vulkan_api_v1_draw(render::api::v1::CommandBuffer cb, uint32_t vertices, uint32_t instances) noexcept
    {
        vkCmdDraw(command_buffer(cb), vertices, instances, 0, 0);
    }

    void vulkan_api_v1_renderpass_end(CommandBuffer cb) noexcept
    {
        vkCmdEndRenderPass(command_buffer(cb));
    }

    void vulkan_api_v1_command_end(CommandBuffer cb) noexcept
    {
        vkEndCommandBuffer(command_buffer(cb));
    }

    void init_api(void* ptr) noexcept
    {
        auto instance = reinterpret_cast<render::api::v1::RenderInterface*>(ptr);
        instance->check_func = vulkan_api_v1_initialized;
        instance->vertex_buffer_map_data = vulkan_api_v1_buffer_map;
        instance->vertex_buffer_unmap_data = vulkan_api_v1_buffer_unmap;
        instance->uniform_buffer_map_data = vulkan_api_v1_uniform_buffer_map;
        instance->uniform_buffer_unmap_data = vulkan_api_v1_uniform_buffer_unmap;

        instance->cmd_begin_func = vulkan_api_v1_command_begin;
        instance->cmd_begin_renderpass_func = vulkan_api_v1_renderpass_begin;
        instance->cmd_bind_render_pipeline_func = vulkan_api_v1_bind_render_pipeline;
        instance->cmd_bind_descriptor_sets_func = vulkan_api_v1_bind_descriptor_sets;
        instance->cmd_bind_vertex_buffers_func = vulkan_api_v1_bind_vertex_buffers;
        instance->cmd_set_viewport_func = vulkan_api_v1_set_viewport;
        instance->cmd_set_scissor_func = vulkan_api_v1_set_scissor;
        instance->cmd_draw_func = vulkan_api_v1_draw;
        instance->cmd_end_renderpass_func = vulkan_api_v1_renderpass_end;
        instance->cmd_end_func = vulkan_api_v1_command_end;
    }

} // namespace render::api::v1::vulkan
