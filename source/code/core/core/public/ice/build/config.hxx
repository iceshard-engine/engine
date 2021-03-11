#pragma once
#include <string_view>

namespace ice::build
{

    enum class Configuration : uint32_t
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
#define ICE_DEBUG  1

#elif defined ICE_DEVELOP

    static constexpr Configuration current_config = Configuration::Develop;

#undef ICE_DEVELOP
#define ICE_DEVELOP  1

#elif defined ICE_PROFILE

    static constexpr Configuration current_config = Configuration::Profile;

#undef ICE_PROFILE
#define ICE_PROFILE  1

#elif defined ICE_RELEASE

    static constexpr Configuration current_config = Configuration::Release;

#undef ICE_RELEASE
#define ICE_RELEASE  1

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
