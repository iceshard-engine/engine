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


#if defined ISC_DEBUG

    static constexpr Configuration current_config = Configuration::Debug;

#undef ISC_DEBUG
#define ISC_DEBUG  1

#elif defined ISC_DEVELOP

    static constexpr Configuration current_config = Configuration::Develop;

#undef ISC_DEVELOP
#define ISC_DEVELOP  1

#elif defined ISC_PROFILE

    static constexpr Configuration current_config = Configuration::Profile;

#undef ISC_PROFILE
#define ISC_PROFILE  1

#elif defined ISC_RELEASE

    static constexpr Configuration current_config = Configuration::Release;

#undef ISC_RELEASE
#define ISC_RELEASE  1

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
