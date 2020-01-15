#pragma once
#include <core/base.hxx>
#include <core/cexpr/stringid.hxx>

namespace render::api
{

    namespace v1
    {

        void assert_render_api() noexcept;

        static constexpr auto version_name = "v1"_sid;

        enum class Framebuffer : uintptr_t { };

        enum class RenderPass : uintptr_t { };

        enum class RenderPipeline : uintptr_t { };

        enum class DescriptorSets : uintptr_t { };

        enum class VertexBuffer : uintptr_t { };

        enum class UniformBuffer : uintptr_t { };

        enum class CommandBuffer : uintptr_t { };

        enum class Texture : uintptr_t { };

        struct BufferDataView
        {
            void* data_pointer;
            uint32_t data_size;
        };

        struct RenderInterface
        {
            void (*check_func)();
            void (*vertex_buffer_map_data)(VertexBuffer, BufferDataView&);
            void (*vertex_buffer_unmap_data)(VertexBuffer);
            void (*uniform_buffer_map_data)(UniformBuffer, BufferDataView&);
            void (*uniform_buffer_unmap_data)(UniformBuffer);

            // Commands
            void (*cmd_begin_func)(CommandBuffer);
            void (*cmd_begin_renderpass_func)(CommandBuffer);
            void (*cmd_bind_render_pipeline_func)(CommandBuffer, RenderPipeline);
            void (*cmd_bind_descriptor_sets_func)(CommandBuffer, DescriptorSets);
            void (*cmd_bind_vertex_buffers_func)(CommandBuffer, VertexBuffer, VertexBuffer);
            void (*cmd_bind_index_buffers_func)(CommandBuffer, VertexBuffer);
            void (*cmd_set_viewport_func)(CommandBuffer, uint32_t, uint32_t);
            void (*cmd_set_scissor_func)(CommandBuffer, uint32_t, uint32_t);
            void (*cmd_set_scissor2_func)(CommandBuffer, uint32_t, uint32_t, uint32_t, uint32_t);
            void (*cmd_draw_func)(CommandBuffer, uint32_t, uint32_t);
            void (*cmd_draw_indexed_func)(CommandBuffer, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t);
            void (*cmd_end_renderpass_func)(CommandBuffer);
            void (*cmd_end_func)(CommandBuffer);

            void* reserved[32];
        };

        extern RenderInterface* render_api_instance;

    } // namespace v1

    using namespace render::api::v1;

} // namespace render::api
