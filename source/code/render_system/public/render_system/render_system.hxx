#pragma once
#include <render_system/render_command_buffer.hxx>

namespace render
{

    //! \brief Render context interface.
    class RenderContext;

    //! \brief A render system interface.
    class RenderSystem
    {
    public:
        virtual ~RenderSystem() noexcept = default;

        //! \brief Creates a new render context on the current thread.
        //!
        //! \remarks On some backends the thread, where the context was created, is final and the context cannot be used outside of it.
        //virtual auto create_render_context() noexcept -> RenderContext* = 0;

        virtual auto command_buffer() noexcept -> render::RenderCommandBuffer& = 0;

        virtual void swap() noexcept = 0;
    };

} // namespace render
