#pragma once

namespace core::build::platform
{

//! \brief Defines all supported systems.
enum class System { Windows };

//! \brief Defines all supported architectures.
enum class Architecture { x64 };

//! \brief Defines all supported compilers.
enum class Compiler { MSVC };

//! \brief Returns the name of the given system.
auto to_string(System type) noexcept -> const char*;

//! \brief Returns the name of the given architectrure.
auto to_string(Architecture type) noexcept -> const char*;

//! \brief Returns the name of the given Compiler.
auto to_string(Compiler type) noexcept -> const char*;


//! \brief Defines a single platform.
struct Platform
{
    //! \brief The platform name.
    const char* name;

    //! \brief The platform system.
    System system;

    //! \brief The platform system.
    Architecture architecture;

    //! \brief The platform system.
    Compiler compiler;
};

//! \brief Returns the name of the given platform.
auto to_string(Platform type) noexcept -> const char*;


/// Operators ///


constexpr bool operator==(const Platform& left, const Platform& right) noexcept;
constexpr bool operator!=(const Platform& left, const Platform& right) noexcept;

constexpr bool operator==(const Platform& left, System right) noexcept;
constexpr bool operator!=(const Platform& left, System right) noexcept;

constexpr bool operator==(const Platform& left, Architecture right) noexcept;
constexpr bool operator!=(const Platform& left, Architecture right) noexcept;

constexpr bool operator==(const Platform& left, Compiler right) noexcept;
constexpr bool operator!=(const Platform& left, Compiler right) noexcept;


/// Common platforms ///


static constexpr Platform platform_windows{ "windows-x64-msvc", System::Windows, Architecture::x64, Compiler::MSVC };


/// All common platforms ///


static constexpr Platform all_platforms[] = { platform_windows };


/// Current ///


#if defined(_WIN64)
static constexpr auto current_platform = platform_windows;
#else
static_assert(false, "Unknow platform!")
#endif


/// Inline definitions ///


#include "platform.inl"

} // namespace core::build::configuration
