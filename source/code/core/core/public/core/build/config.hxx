#pragma once

namespace core::build::configuration
{


    //! \brief Supported configurations.
    enum class ConfigurationType { Debug, Develop, Profile, Release };

    //! \brief A structure holding information about a specific configuration.
    struct Configuration
    {
        //! \brief Configuration name.
        const char* name;

        //! \brief Configuration type.
        ConfigurationType type;
    };


    //! \brief Returns the Configuration name.
    auto to_string(Configuration type) noexcept -> const char*;

    //! \brief Returns the ConfigurationType name.
    auto to_string(ConfigurationType type) noexcept -> const char*;


    constexpr bool operator==(Configuration left, Configuration right) noexcept;
    constexpr bool operator!=(Configuration left, Configuration right) noexcept;

    constexpr bool operator==(Configuration left, ConfigurationType right) noexcept;
    constexpr bool operator!=(Configuration left, ConfigurationType right) noexcept;


#if defined ISC_DEBUG

    static constexpr Configuration current_config{ "Debug",         ConfigurationType::Debug };

#undef ISC_DEBUG
#define ISC_DEBUG  1

#elif defined ISC_DEVELOP

    static constexpr Configuration current_config{ "Develop",       ConfigurationType::Develop };

#undef ISC_DEVELOP
#define ISC_DEVELOP  1

#elif defined ISC_PROFILE

    static constexpr Configuration current_config{ "Profile",       ConfigurationType::Profile };

#undef ISC_PROFILE
#define ISC_PROFILE  1

#elif defined ISC_RELEASE

    static constexpr Configuration current_config{ "Release",       ConfigurationType::Release };

#undef ISC_RELEASE
#define ISC_RELEASE  1

#else

    static_assert(false, "Unknown configuration!");

#endif


#include "config.inl"


} // namespace core::build::configuration
