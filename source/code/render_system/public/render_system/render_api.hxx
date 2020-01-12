#pragma once
#include <core/base.hxx>
#include <core/cexpr/stringid.hxx>

namespace render::api
{

    namespace v1
    {

        void assert_render_api() noexcept;

        constexpr auto version_name = core::cexpr::stringid_cexpr("v1");

        enum class CommandBuffer : uintptr_t;

        enum class RenderPipeline : uintptr_t;

        enum class VertexBuffer : uintptr_t;

        struct RenderInterface
        {
            void (*check_func)();
            void (*vertex_buffer_map_data)(VertexBuffer);
            void (*vertex_buffer_unmap_data)(VertexBuffer);

            // Commands
            void (*cmd_begin_func)(CommandBuffer);
            void (*cmd_end_func)(CommandBuffer);

            void* reserved[32];
        };

        extern RenderInterface* render_api_instance;

    } // namespace v1

    using namespace render::api::v1;

    struct BufferDataView
    {
        void* data_pointer;
        uint32_t data_size;
    };

} // namespace render::api
