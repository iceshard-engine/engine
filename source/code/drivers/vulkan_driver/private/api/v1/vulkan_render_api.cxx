#include "vulkan_render_api.hxx"
#include "../../vulkan_buffer.hxx"
#include "../../vulkan_image.hxx"
#include <core/memory.hxx>
#include <core/allocators/stack_allocator.hxx>
#include "../../vulkan_device_memory_manager.hxx"
#include <iceshard/renderer/vulkan/vulkan_command_buffer.hxx>
#include <iceshard/renderer/vulkan/vulkan_resources.hxx>
#include <iceshard/renderer/vulkan/vulkan_system.hxx>

#include <vulkan/vulkan.h>

namespace iceshard::renderer::api::v1_1::vulkan
{
    using namespace iceshard::renderer::vulkan;

    auto command_buffer(CommandBuffer command_buffer) noexcept -> VkCommandBuffer
    {
        return VulkanCommandBuffer{ command_buffer }.native;
    }

    auto render_system_instance() -> iceshard::renderer::vulkan::VulkanRenderSystem&
    {
        auto* system = reinterpret_cast<iceshard::renderer::vulkan::VulkanRenderSystem*>(
            render_module_api->internal_data
        );
        IS_ASSERT(system != nullptr, "Vulkan render system module initialization error!");

        return *system;
    }

    void vulkan_api_v1_initialized()
    {
        fmt::print("Using Vulkan Render API v1.\n");
    }

    auto vulkan_api_v1_create_texture(core::stringid_arg_type name, core::math::vec2u size) -> api::Texture
    {
        auto& render_system = render_system_instance();

        return render_system.textures().allocate_texture(name, { size.x, size.y });
    };

    auto vulkan_api_v1_create_buffer(BufferType type, uint32_t size) -> api::Buffer
    {
        return render_system_instance().create_data_buffer(type, size);
    }

    void vulkan_api_v1_create_command_buffers(CommandBufferType type, CommandBuffer* buffers, uint32_t size)
    {
        auto* const buffers_native = reinterpret_cast<VkCommandBuffer*>(buffers);

        core::pod::Array<VkCommandBuffer> array_view{ core::memory::globals::null_allocator() };
        core::pod::array::create_view(array_view, buffers_native, size);

        render_system_instance().command_buffer_pool().allocate_buffers(
            type,
            array_view
        );
    }

    void vulkan_api_v1_cmd_begin(CommandBuffer cb, CommandBufferUsage usage)
    {
        VkCommandBufferBeginInfo beginInfo = {};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        beginInfo.flags = 0;

        if (usage == CommandBufferUsage::RunOnce)
        {
            beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
        }

        vkBeginCommandBuffer(command_buffer(cb), &beginInfo);
    }

    void vulkan_api_v1_cmd_begin_subpass(CommandBuffer cb, RenderPass renderpass, uint32_t subpass)
    {
        VkCommandBufferInheritanceInfo inheritance_info;
        inheritance_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_INHERITANCE_INFO;
        inheritance_info.pNext = nullptr;
        inheritance_info.framebuffer = nullptr;
        inheritance_info.renderPass = reinterpret_cast<VkRenderPass>(renderpass);
        inheritance_info.queryFlags = 0;
        inheritance_info.occlusionQueryEnable = VK_FALSE;
        inheritance_info.pipelineStatistics = 0;
        inheritance_info.subpass = subpass;

        VkCommandBufferBeginInfo begin_info = {};
        begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        begin_info.flags = VK_COMMAND_BUFFER_USAGE_RENDER_PASS_CONTINUE_BIT;
        begin_info.pInheritanceInfo = &inheritance_info;
        begin_info.pNext = nullptr;

        vkBeginCommandBuffer(command_buffer(cb), &begin_info);
    }

    void vulkan_api_v1_cmd_end(CommandBuffer cb)
    {
        vkEndCommandBuffer(command_buffer(cb));
    }

    void vulkan_api_v1_update_texture(api::CommandBuffer cb, api::Texture texture, api::Buffer buffer, core::math::vec2u size)
    {
        auto native_cb = command_buffer(cb);
        auto image_handle = reinterpret_cast<VulkanImage*>(texture);
        auto buffer_handle = reinterpret_cast<VulkanBuffer*>(buffer);

        VkImageMemoryBarrier barrier = {};
        barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.image = image_handle->native_handle();
        barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        barrier.subresourceRange.baseMipLevel = 0;
        barrier.subresourceRange.levelCount = 1;
        barrier.subresourceRange.baseArrayLayer = 0;
        barrier.subresourceRange.layerCount = 1;
        barrier.srcAccessMask = 0; // TODO
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT; // TODO

        vkCmdPipelineBarrier(
            native_cb,
            VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
            0,
            0, nullptr,
            0, nullptr,
            1, &barrier
        );

        VkBufferImageCopy region = {};
        region.bufferOffset = 0;
        region.bufferRowLength = 0;
        region.bufferImageHeight = 0;

        region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        region.imageSubresource.mipLevel = 0;
        region.imageSubresource.baseArrayLayer = 0;
        region.imageSubresource.layerCount = 1;

        region.imageOffset = { 0, 0, 0 };
        region.imageExtent.width = size.x;
        region.imageExtent.height = size.y;
        region.imageExtent.depth = 1;
        vkCmdCopyBufferToImage(
            native_cb,
            buffer_handle->native_handle(),
            image_handle->native_handle(),
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            1,
            &region
        );


        barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

        vkCmdPipelineBarrier(
            native_cb,
            VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
            0,
            0, nullptr,
            0, nullptr,
            1, &barrier
        );
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

    void vulkan_api_v1_next_render_subpass(
        CommandBuffer cb,
        RenderSubPass render_subpass
    ) noexcept
    {
        VkSubpassContents contents = VkSubpassContents::VK_SUBPASS_CONTENTS_MAX_ENUM;
        switch (render_subpass)
        {
        case iceshard::renderer::api::v1_1::types::RenderSubPass::Commands:
            contents = VkSubpassContents::VK_SUBPASS_CONTENTS_INLINE;
            break;
        case iceshard::renderer::api::v1_1::types::RenderSubPass::CommandBuffers:
            contents = VkSubpassContents::VK_SUBPASS_CONTENTS_SECONDARY_COMMAND_BUFFERS;
            break;
        }

        vkCmdNextSubpass(command_buffer(cb), contents);
    }

    void vulkan_api_v1_execute_command_buffers(
        CommandBuffer cb,
        uint32_t count,
        CommandBuffer* buffers
    ) noexcept
    {
        auto* const buffers_native = reinterpret_cast<VkCommandBuffer*>(buffers);

        vkCmdExecuteCommands(command_buffer(cb), count, buffers_native);
    }

    void vulkan_api_v1_push_constants(types::CommandBuffer cb, api::Pipeline pipeline, uint32_t offset, uint32_t size, void const* data) noexcept
    {
        const auto* vulkan_pipeline = reinterpret_cast<iceshard::renderer::vulkan::VulkanPipeline const*>(pipeline);
        vkCmdPushConstants(
            command_buffer(cb),
            vulkan_pipeline->layout,
            VK_SHADER_STAGE_VERTEX_BIT,
            offset, size, data
        );
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
        auto instance = reinterpret_cast<RenderModuleInterface*>(ptr);
        instance->check_func = vulkan_api_v1_initialized;
        instance->create_texture_func = vulkan_api_v1_create_texture;
        instance->create_data_buffer_func = vulkan_api_v1_create_buffer;
        instance->create_command_buffers_func = vulkan_api_v1_create_command_buffers;
        instance->buffer_array_map_data_func = vulkan_api_v1a_buffer_array_map;
        instance->buffer_array_unmap_data_func = vulkan_api_v1a_buffer_array_unmap;


        instance->cmd_begin_func = vulkan_api_v1_cmd_begin;
        instance->cmd_begin_subpass_func = vulkan_api_v1_cmd_begin_subpass;
        instance->cmd_end_func = vulkan_api_v1_cmd_end;

        instance->cmd_update_texture_func = vulkan_api_v1_update_texture;

        //instance->cmd_begin_render_pass_func = vulkan_api_v1_begin_render_pass;
        instance->cmd_next_subpass_func = vulkan_api_v1_next_render_subpass;
        instance->cmd_execute_commands_func = vulkan_api_v1_execute_command_buffers;
        //instance->cmd_end_render_pass_func = vulkan_api_v1_end_render_pass;

        instance->cmd_push_constants_func = vulkan_api_v1_push_constants;
        instance->cmd_bind_render_pipeline_func = vulkan_api_v1_bind_render_pipeline;
        instance->cmd_bind_resource_set_func = vulkan_api_v1_bind_resource_set;
        instance->cmd_bind_vertex_buffers_array_func = vulkan_api_v1_bind_vertex_buffers_array;
        instance->cmd_bind_index_buffer_func = vulkan_api_v1_bind_index_buffer;
        instance->cmd_set_viewport_func = vulkan_api_v1_set_viewport;
        instance->cmd_set_scissor_func = vulkan_api_v1_set_scissor;
        instance->cmd_set_scissor2_func = vulkan_api_v1_set_scissor2;
        instance->cmd_draw_func = vulkan_api_v1_draw;
        instance->cmd_draw_indexed_func = vulkan_api_v1_draw_indexed;
    }

} // namespace
