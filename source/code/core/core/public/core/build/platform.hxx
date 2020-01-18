#pragma once

namespace core::build::platform
{

    //! \brief Supported systems.
    enum class System
    {
        Windows
    };

    //! \brief Supported architectures.
    enum class Architecture
    {
        x64
    };

    //! \brief Supported compilers.
    enum class Compiler
    {
        MSVC
    };

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

    //! \brief Returns the System name.
    auto to_string(System type) noexcept -> const char*;

    //! \brief Returns the Architecture name.
    auto to_string(Architecture type) noexcept -> const char*;

    //! \brief Returns the Compiler name.
    auto to_string(Compiler type) noexcept -> const char*;

    constexpr bool operator==(const Platform& left, const Platform& right) noexcept;
    constexpr bool operator!=(const Platform& left, const Platform& right) noexcept;

    constexpr bool operator==(const Platform& left, System right) noexcept;
    constexpr bool operator!=(const Platform& left, System right) noexcept;

    constexpr bool operator==(const Platform& left, Architecture right) noexcept;
    constexpr bool operator!=(const Platform& left, Architecture right) noexcept;

    constexpr bool operator==(const Platform& left, Compiler right) noexcept;
    constexpr bool operator!=(const Platform& left, Compiler right) noexcept;

    static constexpr Platform platform_windows{ "windows-x64-msvc", System::Windows, Architecture::x64, Compiler::MSVC };

    static constexpr Platform all_platforms[] = { platform_windows };

#if defined(_WIN64)
#    define ISP_WINDOWS 1
    static constexpr auto current_platform = platform_windows;
#else
#    define ISP_WINDOWS 0
    static_assert(false, "Unknow platform!")
#endif

#include "platform.inl"

} // namespace core::build::platform
