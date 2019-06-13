#pragma once

namespace core::build::configuration
{

//! \brief Defines all supported configuration names.
enum class ConfigurationType { Debug, ReleaseDebug, Release };

//! \brief Returns the name of the ConfigurationType
auto to_string(ConfigurationType type) noexcept -> const char*;


//! \brief Defines a single configuration.
struct Configuration
{
    //! \brief The configuration name.
    const char* name;

    //! \brief The configuration type.
    ConfigurationType type;
};

//! \brief Returns the name of the Configuration
auto to_string(Configuration type) noexcept -> const char*;


/// Operators ///


constexpr bool operator==(const Configuration& left, const Configuration& right) noexcept;
constexpr bool operator!=(const Configuration& left, const Configuration& right) noexcept;

constexpr bool operator==(const Configuration& left, ConfigurationType right) noexcept;
constexpr bool operator!=(const Configuration& left, ConfigurationType right) noexcept;


/// Current ///


#if defined(_DEBUG)
static constexpr Configuration current_config{ "Debug",         ConfigurationType::Debug };
#elif defined(_RDEBUG)
static constexpr Configuration current_config{ "ReleaseDebug",  ConfigurationType::ReleaseDebug };
#elif defined(_NDEBUG)
static constexpr Configuration current_config{ "Release",       ConfigurationType::Release };
#else
static_assert(false, "Unknow configuration!")
#endif


/// Inline definitions ///


#include "config.inl"

} // namespace core::build::configuration
