#pragma once
#include <core/cexpr/stringid.hxx>

namespace iceshard::renderer::api
{

    namespace v1_1
    {

        constexpr auto version_name = "v1.1"_sid;

        inline namespace types
        {

            enum class RenderPass : uintptr_t { Invalid = 0x0 };

            enum class Pipeline : uintptr_t { Invalid = 0x0 };

            enum class CommandBuffer : uintptr_t { Invalid = 0x0 };

            enum class ResourceSet : uintptr_t { Invalid = 0x0 };

            enum class Texture : uintptr_t { Invalid = 0x0 };

            enum class Buffer : uintptr_t { Invalid = 0x0 };

            enum class BufferType : uint32_t
            {
                IndexBuffer,
                VertexBuffer,
                UniformBuffer,
            };

            enum class Sampler : uint32_t
            {
                Default,
                Invalid = std::numeric_limits<std::underlying_type_t<Sampler>>::max()
            };

            struct DataView
            {
                void* data;
                uint32_t size;
            };

        } // namespace types

        inline namespace funcs
        {

            void assert_render_api() noexcept;

        } // namespace funcs

        struct RenderInterface
        {
            // Resource handing
            void(*check_func)();
            void(*buffer_map_data)(types::Buffer, types::DataView&);
            void(*buffer_unmap_data)(types::Buffer);
            void(*buffer_array_map_data)(types::Buffer*, types::DataView*, uint32_t);
            void(*buffer_array_unmap_data)(types::Buffer*, uint32_t);

            // Render commands
            void(*cmd_bind_render_pipeline_func)(types::CommandBuffer, types::Pipeline);
            void(*cmd_bind_resource_set_func)(types::CommandBuffer, types::ResourceSet);
            void(*cmd_bind_vertex_buffers_array_func)(types::CommandBuffer, types::Buffer const*, uint32_t);
            void(*cmd_bind_index_buffer_func)(types::CommandBuffer, types::Buffer);
            void(*cmd_set_viewport_func)(types::CommandBuffer, uint32_t, uint32_t);
            void(*cmd_set_scissor_func)(types::CommandBuffer, uint32_t, uint32_t);
            void(*cmd_set_scissor2_func)(types::CommandBuffer, uint32_t, uint32_t, uint32_t, uint32_t);
            void(*cmd_draw_func)(types::CommandBuffer, uint32_t, uint32_t);
            void(*cmd_draw_indexed_func)(types::CommandBuffer, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t);
            void(*cmd_next_subpass_func)(types::CommandBuffer);

            // Reserved
            void* reserved[32];
        };

        extern RenderInterface* render_api_instance;

    } // namespace v1_1

    using namespace v1_1;

} // namespace iceshard::renderer::api::v1
