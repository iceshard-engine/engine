#pragma once

namespace render
{

    //! \brief A single render context object.
    class RenderContext
    {
    public:
        virtual ~RenderContext() noexcept = default;
    };

} // namespace render
