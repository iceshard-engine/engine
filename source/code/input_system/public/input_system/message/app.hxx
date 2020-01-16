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

    enum class AppStatus
    {
        Unknown,
        InForeground,
        InBackground,
    };

    struct AppStatusChanged
    {
        static constexpr auto message_type = "App.Status.Changed"_sid;

        AppStatus status;
    };

    struct AppStatusChangeing
    {
        static constexpr auto message_type = "App.Status.Changeing"_sid;

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
