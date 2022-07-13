#pragma once
#include <ice/types.hxx>
#include <string_view>

namespace ice::build
{

    enum class Configuration : ice::u8
    {
        Debug,
        Develop,
        Profile,
        Release,
    };

    constexpr auto to_string(Configuration type) noexcept -> std::string_view;


#if defined ICE_DEBUG

    static constexpr Configuration current_config = Configuration::Debug;

#undef ICE_DEBUG
#define ICE_DEBUG 1
#define ICE_DEVELOP 0
#define ICE_PROFILE 0
#define ICE_RELEASE 0

#elif defined ICE_DEVELOP

    static constexpr Configuration current_config = Configuration::Develop;

#undef ICE_DEVELOP
#define ICE_DEBUG 0
#define ICE_DEVELOP 1
#define ICE_PROFILE 0
#define ICE_RELEASE 0

#elif defined ICE_PROFILE

    static constexpr Configuration current_config = Configuration::Profile;

#undef ICE_PROFILE
#define ICE_DEBUG 0
#define ICE_DEVELOP 0
#define ICE_PROFILE 1
#define ICE_RELEASE 0

#elif defined ICE_RELEASE

    static constexpr Configuration current_config = Configuration::Release;

#undef ICE_RELEASE
#define ICE_DEBUG 0
#define ICE_DEVELOP 0
#define ICE_PROFILE 0
#define ICE_RELEASE 1

#else

    static_assert(false, "Unknown configuration!");

#endif


    constexpr auto to_string(Configuration type) noexcept -> std::string_view
    {
        switch (type)
        {
        case Configuration::Debug:
            return "Debug";
        case Configuration::Develop:
            return "Develop";
        case Configuration::Profile:
            return "Profile";
        case Configuration::Release:
            return "Release";
        default:
            return "<invalid>";
        }
    }

} // namespace ice::build
