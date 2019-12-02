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

    enum class AppStatus
    {
        Unknown,
        InForeground,
        InBackground,
    };

    struct AppStatusChanged
    {
        static inline const core::cexpr::stringid_type message_type{ core::cexpr::stringid("App.Status.Changed") };

        AppStatus status;
    };

    struct AppStatusChangeing
    {
        static inline const core::cexpr::stringid_type message_type{ core::cexpr::stringid("App.Status.Changeing") };

        AppStatus new_status;
    };

    inline constexpr auto to_string(AppStatus status) noexcept -> std::string_view
    {
        if (status == AppStatus::InBackground)
        {
            return "InBackground";
        }
        else if (status == AppStatus::InForeground)
        {
            return "InForeground";
        }
        else
        {
            return "Unknown";
        }
    }

} // namespace input::message
