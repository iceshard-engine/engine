#include "vulkan_render_api.hxx"
#include "../../vulkan_buffer.hxx"
#include "../../pipeline/vulkan_descriptor_sets.hxx"
#include <core/memory.hxx>
#include <core/allocators/stack_allocator.hxx>
#include "../../vulkan_device_memory_manager.hxx"
#include <iceshard/renderer/vulkan/vulkan_command_buffer.hxx>
#include <iceshard/renderer/vulkan/vulkan_resources.hxx>

#include <vulkan/vulkan.h>

namespace render::api::v1::vulkan
{

    auto native(CommandBuffer command_buffer) noexcept -> render::api::v1::vulkan::CommandBufferContext const*
    {
        return reinterpret_cast<render::api::v1::vulkan::CommandBufferContext*>(command_buffer);
    }

    auto command_buffer(CommandBuffer command_buffer) noexcept -> VkCommandBuffer
    {
        iceshard::renderer::vulkan::ApiCommandBuffer cmd_buff{ command_buffer };
        return cmd_buff.native;
    }

    void vulkan_api_v1_initialized() noexcept
    {
        fmt::print("Using Vulkan Render API v1.\n");
    }

    void vulkan_api_v1a_buffer_map(render::api::v1::Buffer buffer, BufferDataView& buffer_data)
    {
        reinterpret_cast<render::vulkan::VulkanBuffer*>(buffer)->map_memory(buffer_data);
    }

    void vulkan_api_v1a_buffer_array_map(render::api::v1::Buffer* buffers, BufferDataView* views, uint32_t size)
    {
        auto& mem_manager = reinterpret_cast<render::vulkan::VulkanBuffer*>(buffers[0])->memory_manager();

        core::memory::stack_allocator<sizeof(render::vulkan::VulkanMemoryInfo) * 8> temp_alloc;
        core::pod::Array<render::vulkan::VulkanMemoryInfo> memory_ranges{ temp_alloc };
        core::pod::array::reserve(memory_ranges, 8);

        for (uint32_t idx = 0; idx < size; ++idx)
        {
            core::pod::array::push_back(memory_ranges, reinterpret_cast<render::vulkan::VulkanBuffer*>(buffers[idx])->memory_info());
        }

        mem_manager.map_memory(core::pod::array::begin(memory_ranges), views, size);
    }

    void vulkan_api_v1a_buffer_unmap(render::api::v1::Buffer buffer)
    {
        reinterpret_cast<render::vulkan::VulkanBuffer*>(buffer)->unmap_memory();
    }

    void vulkan_api_v1a_buffer_array_unmap(render::api::v1::Buffer* buffers, uint32_t size)
    {
        auto& mem_manager = reinterpret_cast<render::vulkan::VulkanBuffer*>(buffers[0])->memory_manager();

        core::memory::stack_allocator<sizeof(render::vulkan::VulkanMemoryInfo) * 8> temp_alloc;
        core::pod::Array<render::vulkan::VulkanMemoryInfo> memory_ranges{ temp_alloc };
        core::pod::array::reserve(memory_ranges, 8);

        for (uint32_t idx = 0; idx < size; ++idx)
        {
            core::pod::array::push_back(memory_ranges, reinterpret_cast<render::vulkan::VulkanBuffer*>(buffers[idx])->memory_info());
        }

        mem_manager.unmap_memory(core::pod::array::begin(memory_ranges), size);
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
        auto* command_buffer_context = native(cb);

        VkCommandBufferInheritanceInfo inheritance_info;
        inheritance_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_INHERITANCE_INFO;
        inheritance_info.pNext = nullptr;
        inheritance_info.framebuffer = nullptr;
        inheritance_info.renderPass = command_buffer_context->render_pass_context->renderpass;
        inheritance_info.queryFlags = 0;
        inheritance_info.occlusionQueryEnable = VK_FALSE;
        inheritance_info.pipelineStatistics = 0;
        inheritance_info.subpass = 0;

        VkCommandBufferBeginInfo cmd_buf_info = {};
        cmd_buf_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        cmd_buf_info.pNext = nullptr;
        cmd_buf_info.flags = VK_COMMAND_BUFFER_USAGE_RENDER_PASS_CONTINUE_BIT;
        cmd_buf_info.pInheritanceInfo = &inheritance_info;

        auto api_result = vkBeginCommandBuffer(command_buffer(cb), &cmd_buf_info);
        IS_ASSERT(api_result == VkResult::VK_SUCCESS, "Couldn't begin command buffer.");
    }

    void vulkan_api_v1_1_renderpass_begin(CommandBuffer cb, iceshard::renderer::api::RenderPass renderpass) noexcept
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
        rp_begin.renderPass = reinterpret_cast<VkRenderPass>(renderpass);
        rp_begin.framebuffer = command_buffer_context->render_pass_context->framebuffer;
        rp_begin.renderArea.offset.x = 0;
        rp_begin.renderArea.offset.y = 0;
        rp_begin.renderArea.extent = command_buffer_context->render_pass_context->extent;
        rp_begin.clearValueCount = 2;
        rp_begin.pClearValues = clear_values;

        vkCmdBeginRenderPass(command_buffer_context->command_buffer, &rp_begin, VK_SUBPASS_CONTENTS_INLINE);
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
        const auto* vulkan_resource_set = reinterpret_cast<iceshard::renderer::vulkan::VulkanResourceSet const*>(descriptor_sets);

        vkCmdBindDescriptorSets(
            command_buffer(cb),
            VK_PIPELINE_BIND_POINT_GRAPHICS,
            vulkan_resource_set->pipeline_layout,
            0,
            core::size(vulkan_resource_set->descriptor_sets),
            vulkan_resource_set->descriptor_sets,
            0,
            NULL
        );
    }

    void vulkan_api_v1_bind_vertex_buffers(render::api::v1::CommandBuffer cb, render::api::v1::VertexBuffer vertice, render::api::v1::VertexBuffer instance) noexcept
    {
        auto const* vtx_buff = reinterpret_cast<render::vulkan::VulkanBuffer*>(vertice);
        auto const* ins_buff = reinterpret_cast<render::vulkan::VulkanBuffer*>(instance);

        VkDeviceSize const offsets[2] = {
            0,
            0
        };
        VkBuffer const buffers[2] = {
            vtx_buff->native_handle(),
            ins_buff->native_handle()
        };
        vkCmdBindVertexBuffers(command_buffer(cb), 0, 1, buffers, offsets);
    }

    void vulkan_api_v1_bind_vertex_buffers_array(render::api::v1::CommandBuffer cb, render::api::v1::Buffer const* handles, uint32_t size) noexcept
    {
        core::memory::stack_allocator<(sizeof(VkBuffer) + sizeof(VkDeviceSize)) * 8> stack_alloc;

        core::pod::Array<VkBuffer> native_handles{ stack_alloc };
        core::pod::Array<VkDeviceSize> buffer_offsets{ stack_alloc };
        core::pod::array::reserve(native_handles, 8);
        core::pod::array::reserve(buffer_offsets, 8);

        for (uint32_t idx = 0; idx < size; ++idx)
        {
            auto const& buffer_ptr = *reinterpret_cast<render::vulkan::VulkanBuffer const*>(handles[idx]);
            core::pod::array::push_back(native_handles, buffer_ptr.native_handle());
            core::pod::array::push_back(buffer_offsets, 0llu);
        }

        vkCmdBindVertexBuffers(command_buffer(cb), 0, size, core::pod::array::begin(native_handles), core::pod::array::begin(buffer_offsets));
    }

    void vulkan_api_v1_bind_index_buffers(render::api::v1::CommandBuffer cb, render::api::v1::VertexBuffer indices) noexcept
    {
        vkCmdBindIndexBuffer(command_buffer(cb), reinterpret_cast<render::vulkan::VulkanBuffer*>(indices)->native_handle(), 0, VK_INDEX_TYPE_UINT16);
    }

    void vulkan_api_v1_bind_index_buffer(render::api::v1::CommandBuffer cb, render::api::v1::Buffer buffer) noexcept
    {
        vkCmdBindIndexBuffer(command_buffer(cb), reinterpret_cast<render::vulkan::VulkanBuffer*>(buffer)->native_handle(), 0, VK_INDEX_TYPE_UINT16);
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

    void vulkan_api_v1_set_scissor2(render::api::v1::CommandBuffer cb, uint32_t x, uint32_t y, uint32_t width, uint32_t height) noexcept
    {
        VkRect2D scissor{};
        scissor.extent.width = width;
        scissor.extent.height = height;
        scissor.offset.x = x;
        scissor.offset.y = y;
        vkCmdSetScissor(command_buffer(cb), 0, 1, &scissor);
    }

    void vulkan_api_v1_draw(render::api::v1::CommandBuffer cb, uint32_t vertices, uint32_t instances) noexcept
    {
        vkCmdDraw(command_buffer(cb), vertices, instances, 0, 0);
    }

    void vulkan_api_v1_draw_indexed(
        render::api::v1::CommandBuffer cb,
        uint32_t indices,
        uint32_t instances,
        uint32_t base_index,
        uint32_t base_vertex,
        uint32_t base_instance
    ) noexcept
    {
        vkCmdDrawIndexed(command_buffer(cb), indices, instances, base_index, base_vertex, base_instance);
    }

    void vulkan_api_v1_renderpass_next(CommandBuffer cb) noexcept
    {
        vkCmdNextSubpass(command_buffer(cb), VkSubpassContents::VK_SUBPASS_CONTENTS_INLINE);
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
        instance->buffer_map_data = vulkan_api_v1a_buffer_map;
        instance->buffer_unmap_data = vulkan_api_v1a_buffer_unmap;
        instance->buffer_array_map_data = vulkan_api_v1a_buffer_array_map;
        instance->buffer_array_unmap_data = vulkan_api_v1a_buffer_array_unmap;
        instance->vertex_buffer_map_data = vulkan_api_v1_buffer_map;
        instance->vertex_buffer_unmap_data = vulkan_api_v1_buffer_unmap;
        instance->uniform_buffer_map_data = vulkan_api_v1_uniform_buffer_map;
        instance->uniform_buffer_unmap_data = vulkan_api_v1_uniform_buffer_unmap;

        instance->cmd_begin_func = vulkan_api_v1_command_begin;
        instance->cmd_begin_renderpass_func = vulkan_api_v1_renderpass_begin;
        instance->cmd_begin_renderpass_func2 = vulkan_api_v1_1_renderpass_begin;
        instance->cmd_bind_render_pipeline_func = vulkan_api_v1_bind_render_pipeline;
        instance->cmd_bind_descriptor_sets_func = vulkan_api_v1_bind_descriptor_sets;
        instance->cmd_bind_vertex_buffers_func = vulkan_api_v1_bind_vertex_buffers;
        instance->cmd_bind_vertex_buffers_array_func = vulkan_api_v1_bind_vertex_buffers_array;
        instance->cmd_bind_index_buffers_func = vulkan_api_v1_bind_index_buffers;
        instance->cmd_bind_index_buffer_func = vulkan_api_v1_bind_index_buffer;
        instance->cmd_set_viewport_func = vulkan_api_v1_set_viewport;
        instance->cmd_set_scissor_func = vulkan_api_v1_set_scissor;
        instance->cmd_set_scissor2_func = vulkan_api_v1_set_scissor2;
        instance->cmd_draw_func = vulkan_api_v1_draw;
        instance->cmd_draw_indexed_func = vulkan_api_v1_draw_indexed;
        instance->cmd_next_subpass_func = vulkan_api_v1_renderpass_next;
        instance->cmd_end_renderpass_func = vulkan_api_v1_renderpass_end;
        instance->cmd_end_func = vulkan_api_v1_command_end;
    }

} // namespace render::api::v1::vulkan
