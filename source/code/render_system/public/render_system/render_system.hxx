#pragma once
#include <render_system/render_command_buffer.hxx>
#include <render_system/render_vertex_descriptor.hxx>

namespace render
{

    //! \brief Render context interface.
    class RenderContext;

    //! \brief A render system interface.
    class RenderSystem
    {
    public:
        virtual ~RenderSystem() noexcept = default;

        virtual auto command_buffer() noexcept -> CommandBufferHandle = 0;

        virtual auto current_frame_buffer() noexcept -> FrameBufferHandle = 0;

        virtual void create_named_descriptor_set(
            core::cexpr::stringid_argument_type name,
            VertexBinding const& binding,
            VertexDescriptor const* descriptors,
            uint32_t descriptor_count) noexcept = 0;

        template<uint32_t Size>
        void create_named_descriptor_set(
            core::cexpr::stringid_argument_type name,
            VertexDescriptorSet<Size> const& binding_set) noexcept;

        //! \brief Creates a new render context on the current thread.
        //!
        //! \remarks On some backends the thread, where the context was created, is final and the context cannot be used outside of it.
        //virtual auto create_render_context() noexcept -> RenderContext* = 0;

        //virtual auto command_buffer() noexcept -> render::RenderCommandBuffer& = 0;

        virtual void swap() noexcept = 0;
    };

    template<uint32_t Size>
    inline void RenderSystem::create_named_descriptor_set(
        core::cexpr::stringid_argument_type name,
        VertexDescriptorSet<Size> const& binding_set) noexcept
    {
        create_named_descriptor_set(name, binding_set.binding, binding_set.descriptors, Size);
    }

} // namespace render
