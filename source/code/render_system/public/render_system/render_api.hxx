#pragma once
#include <core/base.hxx>
#include <core/cexpr/stringid.hxx>

namespace render::api
{

    namespace v1
    {

        void assert_render_api() noexcept;

        constexpr auto version_name = core::cexpr::stringid_cexpr("v1");

        enum class CommandBuffer : uintptr_t { };

        enum class RenderPipeline : uintptr_t { };

        enum class VertexBuffer : uintptr_t { };

        enum class UniformBuffer : uintptr_t { };

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
            void (*cmd_end_func)(CommandBuffer);

            void* reserved[32];
        };

        extern RenderInterface* render_api_instance;

    } // namespace v1

    using namespace render::api::v1;

} // namespace render::api
