#pragma once

namespace core::build::platform
{


    /// Types and Functions ///


    //! \brief Supported systems.
    enum class System { Windows };

    //! \brief Supported architectures.
    enum class Architecture { x64 };

    //! \brief Supported compilers.
    enum class Compiler { MSVC };

    //! \brief Returns the System name.
    auto to_string(System type) noexcept -> const char*;

    //! \brief Returns the Architecture name.
    auto to_string(Architecture type) noexcept -> const char*;

    //! \brief Returns the Compiler name.
    auto to_string(Compiler type) noexcept -> const char*;

    //! \brief A structure holding information about a specific platform.
    struct Platform
    {
        //! \brief Platform name.
        const char* name;

        //! \brief Platform system.
        System system;

        //! \brief Platform architecture.
        Architecture architecture;

        //! \brief Platform compiler.
        Compiler compiler;
    };

    //! \brief Returns the Platform name.
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


    /// All platforms ///


    static constexpr Platform all_platforms[] = { platform_windows };


    /// Platform detection ///


#if defined(_WIN64)
    static constexpr auto current_platform = platform_windows;
#else
    static_assert(false, "Unknow platform!")
#endif


        /// Inline definitions ///


#include "platform.inl"


} // namespace core::build::platform
