#pragma once
#include <core/base.hxx>
#include <core/cexpr/stringid.hxx>

namespace input::message
{

    //! \brief A message send when the application requested to exit.
    struct AppExit
    {
        static constexpr auto message_type = "App.Exit"_sid;

        int exit_code{ 0 };
    };

} // namespace input::message
