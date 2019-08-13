#pragma once

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
        virtual auto create_render_context() noexcept -> RenderContext = 0;
    };


} // namespace render
