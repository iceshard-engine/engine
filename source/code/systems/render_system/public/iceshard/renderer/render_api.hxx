#pragma once
#include <core/cexpr/stringid.hxx>
#include <core/math/vector.hxx>

namespace iceshard::renderer::api
{

    namespace v1_1
    {

        constexpr auto version_name = "v1.1"_sid;

        inline namespace types
        {

            enum class RenderPass : uintptr_t
            {
                Invalid = 0x0
            };

            enum class RenderSubPass : uint32_t
            {
                Commands = 0x0,
                CommandBuffers = 0x1,
            };

            enum class Framebuffer : uintptr_t
            {
                Invalid = 0x0
            };

            enum class Pipeline : uintptr_t
            {
                Invalid = 0x0
            };

            enum class CommandBuffer : uintptr_t
            {
                Invalid = 0x0
            };

            enum class ResourceSet : uintptr_t
            {
                Invalid = 0x0
            };

            enum class Texture : uintptr_t
            {
                Invalid = 0x0,
                Attachment0 = std::numeric_limits<std::underlying_type_t<Texture>>::max() - 0,
                Attachment1 = std::numeric_limits<std::underlying_type_t<Texture>>::max() - 1,
                Attachment2 = std::numeric_limits<std::underlying_type_t<Texture>>::max() - 2,
                Attachment3 = std::numeric_limits<std::underlying_type_t<Texture>>::max() - 3,
            };

            enum class Buffer : uintptr_t
            {
                Invalid = 0x0
            };

            enum class BufferType : uint32_t
            {
                IndexBuffer,
                VertexBuffer,
                UniformBuffer,
                TransferBuffer,
            };

            enum class CommandBufferType : uint32_t
            {
                Primary,
                Secondary,
            };

            enum class CommandBufferUsage : uint32_t
            {
                None = 0x0,
                RunOnce = 0x01,
                SubPass = 0x10,
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

        struct RenderModuleInterface
        {
            void* internal_data;

            // Resource handing
            void(*check_func)();
            auto(*create_texture_func)(core::stringid_arg_type, core::math::vec2u)->types::Texture;
            auto(*create_data_buffer_func)(types::BufferType, core::math::u32) -> types::Buffer;
            void(*create_command_buffers_func)(types::CommandBufferType, types::CommandBuffer*, uint32_t);
            void(*buffer_array_map_data_func)(types::Buffer*, types::DataView*, uint32_t);
            void(*buffer_array_unmap_data_func)(types::Buffer*, uint32_t);

            // Command buffer commands
            void(*cmd_begin_func)(types::CommandBuffer, types::CommandBufferUsage);
            void(*cmd_begin_subpass_func)(types::CommandBuffer, types::RenderPass, uint32_t subpass);
            void(*cmd_end_func)(types::CommandBuffer);

            // Render pass commands
            void(*cmd_begin_render_pass_func)(types::CommandBuffer, types::RenderPass, types::Framebuffer, core::math::vec3f);
            void(*cmd_next_subpass_func)(types::CommandBuffer, types::RenderSubPass);
            void(*cmd_execute_commands_func)(types::CommandBuffer, uint32_t, types::CommandBuffer*);
            void(*cmd_end_render_pass_func)(types::CommandBuffer);

            // Data commands
            void(*cmd_update_texture_func)(types::CommandBuffer, types::Texture, types::Buffer, core::math::vec2u);

            // Render commands
            void(*cmd_push_constants_func)(types::CommandBuffer, types::Pipeline pipeline, uint32_t offset, uint32_t size, void const* data);
            void(*cmd_bind_render_pipeline_func)(types::CommandBuffer, types::Pipeline);
            void(*cmd_bind_resource_set_func)(types::CommandBuffer, types::ResourceSet);
            void(*cmd_bind_vertex_buffers_array_func)(types::CommandBuffer, types::Buffer const*, uint32_t);
            void(*cmd_bind_index_buffer_func)(types::CommandBuffer, types::Buffer);
            void(*cmd_set_viewport_func)(types::CommandBuffer, uint32_t, uint32_t);
            void(*cmd_set_scissor_func)(types::CommandBuffer, uint32_t, uint32_t);
            void(*cmd_set_scissor2_func)(types::CommandBuffer, uint32_t, uint32_t, uint32_t, uint32_t);
            void(*cmd_draw_func)(types::CommandBuffer, uint32_t, uint32_t);
            void(*cmd_draw_indexed_func)(types::CommandBuffer, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t);

            // Reserved
            void* reserved[32];
        };

        extern RenderModuleInterface* render_module_api;

    } // namespace v1_1

    using namespace v1_1;

} // namespace iceshard::renderer::api::v1
