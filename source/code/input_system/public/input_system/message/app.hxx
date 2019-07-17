#pragma once
#include <core/base.hxx>
#include <core/cexpr/stringid.hxx>

namespace input::message
{


    //! \brief A message send when the application requested to exit.
    struct AppExit
    {
        static inline const core::cexpr::stringid_type message_type{ core::cexpr::stringid("App.Exit") };

        int exit_code{ 0 };
    };


} // namespace input::message
