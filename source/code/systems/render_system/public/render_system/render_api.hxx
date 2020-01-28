#pragma once
#include <core/base.hxx>
#include <core/cexpr/stringid.hxx>
#include <iceshard/renderer/render_api.hxx>

namespace render::api
{

    namespace v1
    {

        void assert_render_api() noexcept;

        static constexpr auto version_name = "v1"_sid;

        enum class Framebuffer : uintptr_t { Invalid = 0x0 };

        enum class RenderPass : uintptr_t { Invalid = 0x0 };

        enum class RenderPipeline : uintptr_t { Invalid = 0x0 };

        enum class DescriptorSets : uintptr_t { Invalid = 0x0 };

        enum class VertexBuffer : uintptr_t { Invalid = 0x0 };

        enum class UniformBuffer : uintptr_t { Invalid = 0x0 };

        enum class Buffer : uintptr_t { Invalid = 0x0 };

        enum class BufferType : uint32_t
        {
            IndexBuffer,
            VertexBuffer,
            UniformBuffer,
        };

        using iceshard::renderer::api::CommandBuffer;

        using iceshard::renderer::api::Texture;

        struct BufferDataView
        {
            void* data_pointer;
            uint32_t data_size;
        };

        struct RenderInterface
        {
            void(*check_func)();
            void(*buffer_map_data)(Buffer, BufferDataView&);
            void(*buffer_unmap_data)(Buffer);
            void(*buffer_array_map_data)(Buffer* buffers, BufferDataView* views, uint32_t);
            void(*buffer_array_unmap_data)(Buffer* buffers, uint32_t);
            void(*vertex_buffer_map_data)(VertexBuffer, BufferDataView&);
            void(*vertex_buffer_unmap_data)(VertexBuffer);
            void(*uniform_buffer_map_data)(UniformBuffer, BufferDataView&);
            void(*uniform_buffer_unmap_data)(UniformBuffer);

            // Commands
            void(*cmd_begin_func)(CommandBuffer);

            // [[deprecated("This API function is deprecated!")]]
            void(*cmd_begin_renderpass_func)(CommandBuffer);
            void(*cmd_begin_renderpass_func2)(CommandBuffer, iceshard::renderer::api::RenderPass);
            void(*cmd_bind_render_pipeline_func)(CommandBuffer, RenderPipeline);
            void(*cmd_bind_descriptor_sets_func)(CommandBuffer, DescriptorSets);
            void(*cmd_bind_vertex_buffers_func)(CommandBuffer, VertexBuffer, VertexBuffer);
            void(*cmd_bind_vertex_buffers_array_func)(CommandBuffer, Buffer const*, uint32_t);
            void(*cmd_bind_index_buffers_func)(CommandBuffer, VertexBuffer);
            void(*cmd_bind_index_buffer_func)(CommandBuffer, Buffer);
            void(*cmd_set_viewport_func)(CommandBuffer, uint32_t, uint32_t);
            void(*cmd_set_scissor_func)(CommandBuffer, uint32_t, uint32_t);
            void(*cmd_set_scissor2_func)(CommandBuffer, uint32_t, uint32_t, uint32_t, uint32_t);
            void(*cmd_draw_func)(CommandBuffer, uint32_t, uint32_t);
            void(*cmd_draw_indexed_func)(CommandBuffer, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t);
            void(*cmd_next_subpass_func)(CommandBuffer);
            void(*cmd_end_renderpass_func)(CommandBuffer);
            void(*cmd_end_func)(CommandBuffer);

            void* reserved[32];
        };

        extern RenderInterface* render_api_instance;

    } // namespace v1

    using namespace render::api::v1;

} // namespace render::api
