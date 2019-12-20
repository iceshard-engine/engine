#pragma once

namespace core::build::configuration
{


    //! \brief Supported configurations.
    enum class ConfigurationType { Debug, ReleaseDebug, Release };

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


#if defined(_DEBUG)

    static constexpr Configuration current_config{ "Debug",         ConfigurationType::Debug };

#define ISC_DEBUG 1
#define ISC_RELEASE_DEBUG 0
#define ISC_RELEASE 0

#elif defined(_RDEBUG)

    static constexpr Configuration current_config{ "ReleaseDebug",  ConfigurationType::ReleaseDebug };

#define ISC_DEBUG 0
#define ISC_RELEASE_DEBUG 1
#define ISC_RELEASE 0

#elif defined(_NDEBUG)

    static constexpr Configuration current_config{ "Release",       ConfigurationType::Release };

#define ISC_DEBUG 0
#define ISC_RELEASE_DEBUG 0
#define ISC_RELEASE 1

#else

    static_assert(false, "Unknown configuration!");

#endif


#include "config.inl"


} // namespace core::build::configuration
