#include "vulkan_render_api.hxx"
#include "../../vulkan_buffer.hxx"
#include <core/memory.hxx>
#include <core/allocators/stack_allocator.hxx>
#include "../../vulkan_device_memory_manager.hxx"
#include <iceshard/renderer/vulkan/vulkan_command_buffer.hxx>
#include <iceshard/renderer/vulkan/vulkan_resources.hxx>

#include <vulkan/vulkan.h>

namespace iceshard::renderer::api::v1_1::vulkan
{
    using namespace iceshard::renderer::vulkan;

    auto command_buffer(CommandBuffer command_buffer) noexcept -> VkCommandBuffer
    {
        iceshard::renderer::vulkan::ApiCommandBuffer cmd_buff{ command_buffer };
        return cmd_buff.native;
    }

    void vulkan_api_v1_initialized() noexcept
    {
        fmt::print("Using Vulkan Render API v1.\n");
    }

    void vulkan_api_v1a_buffer_map(Buffer buffer, DataView& buffer_data)
    {
        reinterpret_cast<VulkanBuffer*>(buffer)->map_memory(buffer_data);
    }

    void vulkan_api_v1a_buffer_array_map(Buffer* buffers, DataView* views, uint32_t size)
    {
        auto& mem_manager = reinterpret_cast<VulkanBuffer*>(buffers[0])->memory_manager();

        core::memory::stack_allocator<sizeof(VulkanMemoryInfo) * 8> temp_alloc;
        core::pod::Array<VulkanMemoryInfo> memory_ranges{ temp_alloc };
        core::pod::array::reserve(memory_ranges, 8);

        for (uint32_t idx = 0; idx < size; ++idx)
        {
            core::pod::array::push_back(memory_ranges, reinterpret_cast<VulkanBuffer*>(buffers[idx])->memory_info());
        }

        mem_manager.map_memory(core::pod::array::begin(memory_ranges), views, size);
    }

    void vulkan_api_v1a_buffer_unmap(Buffer buffer)
    {
        reinterpret_cast<VulkanBuffer*>(buffer)->unmap_memory();
    }

    void vulkan_api_v1a_buffer_array_unmap(Buffer* buffers, uint32_t size)
    {
        auto& mem_manager = reinterpret_cast<VulkanBuffer*>(buffers[0])->memory_manager();

        core::memory::stack_allocator<sizeof(VulkanMemoryInfo) * 8> temp_alloc;
        core::pod::Array<VulkanMemoryInfo> memory_ranges{ temp_alloc };
        core::pod::array::reserve(memory_ranges, 8);

        for (uint32_t idx = 0; idx < size; ++idx)
        {
            core::pod::array::push_back(memory_ranges, reinterpret_cast<VulkanBuffer*>(buffers[idx])->memory_info());
        }

        mem_manager.unmap_memory(core::pod::array::begin(memory_ranges), size);
    }

    void vulkan_api_v1_bind_render_pipeline(CommandBuffer cb, Pipeline pipeline) noexcept
    {
        const auto* vulkan_pipeline = reinterpret_cast<iceshard::renderer::vulkan::VulkanPipeline const*>(pipeline);

        vkCmdBindPipeline(command_buffer(cb), VK_PIPELINE_BIND_POINT_GRAPHICS, vulkan_pipeline->pipeline);
    }

    void vulkan_api_v1_bind_resource_set(CommandBuffer cb, ResourceSet resourse_set) noexcept
    {
        const auto* vulkan_resource_set = reinterpret_cast<iceshard::renderer::vulkan::VulkanResourceSet const*>(resourse_set);

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

    void vulkan_api_v1_bind_vertex_buffers_array(CommandBuffer cb, Buffer const* handles, uint32_t size) noexcept
    {
        core::memory::stack_allocator<(sizeof(VkBuffer) + sizeof(VkDeviceSize)) * 8> stack_alloc;

        core::pod::Array<VkBuffer> native_handles{ stack_alloc };
        core::pod::Array<VkDeviceSize> buffer_offsets{ stack_alloc };
        core::pod::array::reserve(native_handles, 8);
        core::pod::array::reserve(buffer_offsets, 8);

        for (uint32_t idx = 0; idx < size; ++idx)
        {
            auto const& buffer_ptr = *reinterpret_cast<VulkanBuffer const*>(handles[idx]);
            core::pod::array::push_back(native_handles, buffer_ptr.native_handle());
            core::pod::array::push_back(buffer_offsets, 0llu);
        }

        vkCmdBindVertexBuffers(command_buffer(cb), 0, size, core::pod::array::begin(native_handles), core::pod::array::begin(buffer_offsets));
    }

    void vulkan_api_v1_bind_index_buffer(CommandBuffer cb, Buffer buffer) noexcept
    {
        vkCmdBindIndexBuffer(command_buffer(cb), reinterpret_cast<VulkanBuffer*>(buffer)->native_handle(), 0, VK_INDEX_TYPE_UINT16);
    }

    void vulkan_api_v1_set_viewport(CommandBuffer cb, uint32_t width, uint32_t height) noexcept
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

    void vulkan_api_v1_set_scissor(CommandBuffer cb, uint32_t width, uint32_t height) noexcept
    {
        VkRect2D scissor{};
        scissor.extent = VkExtent2D{ width, height };
        scissor.offset.x = 0;
        scissor.offset.y = 0;
        vkCmdSetScissor(command_buffer(cb), 0, 1, &scissor);
    }

    void vulkan_api_v1_set_scissor2(CommandBuffer cb, uint32_t x, uint32_t y, uint32_t width, uint32_t height) noexcept
    {
        VkRect2D scissor{};
        scissor.extent.width = width;
        scissor.extent.height = height;
        scissor.offset.x = x;
        scissor.offset.y = y;
        vkCmdSetScissor(command_buffer(cb), 0, 1, &scissor);
    }

    void vulkan_api_v1_draw(CommandBuffer cb, uint32_t vertices, uint32_t instances) noexcept
    {
        vkCmdDraw(command_buffer(cb), vertices, instances, 0, 0);
    }

    void vulkan_api_v1_draw_indexed(
        CommandBuffer cb,
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
        auto instance = reinterpret_cast<RenderInterface*>(ptr);
        instance->check_func = vulkan_api_v1_initialized;
        instance->buffer_map_data = vulkan_api_v1a_buffer_map;
        instance->buffer_unmap_data = vulkan_api_v1a_buffer_unmap;
        instance->buffer_array_map_data = vulkan_api_v1a_buffer_array_map;
        instance->buffer_array_unmap_data = vulkan_api_v1a_buffer_array_unmap;

        instance->cmd_bind_render_pipeline_func = vulkan_api_v1_bind_render_pipeline;
        instance->cmd_bind_resource_set_func = vulkan_api_v1_bind_resource_set;
        instance->cmd_bind_vertex_buffers_array_func = vulkan_api_v1_bind_vertex_buffers_array;
        instance->cmd_bind_index_buffer_func = vulkan_api_v1_bind_index_buffer;
        instance->cmd_set_viewport_func = vulkan_api_v1_set_viewport;
        instance->cmd_set_scissor_func = vulkan_api_v1_set_scissor;
        instance->cmd_set_scissor2_func = vulkan_api_v1_set_scissor2;
        instance->cmd_draw_func = vulkan_api_v1_draw;
        instance->cmd_draw_indexed_func = vulkan_api_v1_draw_indexed;
        instance->cmd_next_subpass_func = vulkan_api_v1_renderpass_next;
    }

} // namespace
