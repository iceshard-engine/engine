#pragma once
#include <core/base.hxx>
#include <core/cexpr/stringid.hxx>

namespace input::message
{

    //! \brief A message send when the application requested to exit.
    struct WindowSizeChanged
    {
        static constexpr auto message_type = "Window.SizeChanged"_sid;

        uint32_t width;
        uint32_t height;
    };

} // namespace input::message
