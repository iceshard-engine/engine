#pragma once

namespace core::build::configuration
{


/// Types and Functions ///


//! \brief Supported configurations.
enum class ConfigurationType { Debug, ReleaseDebug, Release };

//! \brief Returns the ConfigurationType name.
auto to_string(ConfigurationType type) noexcept -> const char*;

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


/// Operators ///


constexpr bool operator==(const Configuration& left, const Configuration& right) noexcept;
constexpr bool operator!=(const Configuration& left, const Configuration& right) noexcept;

constexpr bool operator==(const Configuration& left, ConfigurationType right) noexcept;
constexpr bool operator!=(const Configuration& left, ConfigurationType right) noexcept;


/// Configuration Detection ///


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
