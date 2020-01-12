#include "vulkan_render_api.hxx"
#include "../../vulkan_buffer.hxx"
#include <core/memory.hxx>

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

        auto api_result = vkBeginCommandBuffer(native(cb), &cmd_buf_info);
        IS_ASSERT(api_result == VkResult::VK_SUCCESS, "Couldn't begin command buffer.");
    }

    void vulkan_api_v1_renderpass_begin(render::api::v1::CommandBuffer cb, VkRenderPass renderpass, VkFramebuffer framebuffer, VkExtent2D extent) noexcept
    {
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
        rp_begin.renderPass = renderpass;
        rp_begin.framebuffer = framebuffer;
        rp_begin.renderArea.offset.x = 0;
        rp_begin.renderArea.offset.y = 0;
        rp_begin.renderArea.extent = extent;
        rp_begin.clearValueCount = 2;
        rp_begin.pClearValues = clear_values;

        vkCmdBeginRenderPass(native(cb), &rp_begin, VK_SUBPASS_CONTENTS_INLINE);
    }

    void vulkan_api_v1_bind_pipeline(render::api::v1::CommandBuffer cb, VkPipeline pipeline) noexcept
    {
        vkCmdBindPipeline(native(cb), VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
    }

    void vulkan_api_v1_bind_descriptor_sets(render::api::v1::CommandBuffer cb, VkPipelineLayout pipelinelayout) noexcept
    {
        core::pod::Array<VkDescriptorSet> sets{ core::memory::globals::default_allocator() };
        //_vulkan_descriptor_sets->native_handles(sets);
        vkCmdBindDescriptorSets(
            native(cb),
            VK_PIPELINE_BIND_POINT_GRAPHICS,
            pipelinelayout,
            0,
            core::pod::array::size(sets),
            core::pod::array::begin(sets),
            0,
            NULL
        );
    }


        //{
        //    auto new_view = glm::lookAt(detail::sample::cam_pos, // Camera is at (-5,3,-10), in World Space
        //        glm::vec3(0, 0, 0),                              // and looks at the origin
        //        glm::vec3(0, -1, 0)                              // Head is up (set to 0,-1,0 to look upside-down)
        //    );

        //    static float deg = 0.0f;
        //    deg += 3.0f;
        //    new_view = glm::rotate(new_view, glm::radians(deg), glm::vec3{ 0.f, 1.f, 0.f });

        //    if (deg >= 360.0f)
        //        deg = 0.0f;

        //    auto MVP = detail::sample::clip * detail::sample::projection * new_view;

        //    render::api::BufferDataView data_view;
        //    _vulkan_uniform_buffer->map_memory(data_view);
        //    IS_ASSERT(data_view.data_size >= sizeof(MVP), "Insufficient buffer size!");
        //    memcpy(data_view.data_pointer, &MVP, sizeof(MVP));
        //    _vulkan_uniform_buffer->unmap_memory();
        //}

    void vulkan_api_v1_bind_vertex_buffers(render::api::v1::CommandBuffer cb, VkBuffer vertice, VkBuffer instance) noexcept
    {
        VkDeviceSize const offsets[2] = { 0, 0 };
        VkBuffer const buffers[2] = { vertice, instance };
        vkCmdBindVertexBuffers(native(cb), 0, 2, buffers, offsets);
    }

    void vulkan_api_v1_set_viewport(render::api::v1::CommandBuffer cb, int32_t width, int32_t height) noexcept
    {
        VkViewport viewport{};
        viewport.width = (float)width;
        viewport.height = (float)height;
        viewport.minDepth = (float)0.0f;
        viewport.maxDepth = (float)1.0f;
        viewport.x = 0;
        viewport.y = 0;
        vkCmdSetViewport(native(cb), 0, 1, &viewport);
    }

    void vulkan_api_v1_set_scissor(render::api::v1::CommandBuffer cb, uint32_t width, uint32_t height) noexcept
    {
        VkRect2D scissor{};
        scissor.extent = VkExtent2D{ width, height };
        scissor.offset.x = 0;
        scissor.offset.y = 0;
        vkCmdSetScissor(native(cb), 0, 1, &scissor);
    }

    void vulkan_api_v1_draw(render::api::v1::CommandBuffer cb) noexcept
    {
        vkCmdDraw(native(cb), 12 * 3, 4, 0, 0);
    }

    void vulkan_api_v1_renderpass_end(CommandBuffer cb) noexcept
    {
        vkCmdEndRenderPass(native(cb));
    }

    void vulkan_api_v1_command_end(CommandBuffer cb) noexcept
    {
        vkEndCommandBuffer(native(cb));
    }

    void init_api(void* ptr) noexcept
    {
        auto instace = reinterpret_cast<render::api::v1::RenderInterface*>(ptr);
        instace->check_func = vulkan_api_v1_initialized;
        instace->vertex_buffer_map_data = vulkan_api_v1_buffer_map;
        instace->vertex_buffer_unmap_data = vulkan_api_v1_buffer_unmap;
        instace->uniform_buffer_map_data = vulkan_api_v1_uniform_buffer_map;
        instace->uniform_buffer_unmap_data = vulkan_api_v1_uniform_buffer_unmap;

        instace->cmd_begin_func = vulkan_api_v1_command_begin;
        instace->cmd_end_func = vulkan_api_v1_command_end;
    }

} // namespace render::api::v1::vulkan
