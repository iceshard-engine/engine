/// Copyright 2022 - 2023, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/types.hxx>
#include <string_view>

namespace ice::build
{

    enum class System : ice::u8
    {
        UWP,
        Windows,
        Android,
        Unix
    };

    enum class Architecture : ice::u8
    {
        x86,
        x86_x64,
        Arm64
    };

    enum class Compiler : ice::u8
    {
        MSVC,
        Clang,
        GCC,
    };

    struct Platform
    {
        std::string_view name;

        System system;
        Architecture architecture;
        Compiler compiler;
    };

    constexpr bool operator==(Platform const& left, System right) noexcept;
    constexpr bool operator!=(Platform const& left, System right) noexcept;

    constexpr bool operator==(Platform const& left, Architecture right) noexcept;
    constexpr bool operator!=(Platform const& left, Architecture right) noexcept;

    constexpr bool operator==(Platform const& left, Compiler right) noexcept;
    constexpr bool operator!=(Platform const& left, Compiler right) noexcept;

    constexpr bool operator==(Platform const& left, Platform const& right) noexcept;
    constexpr bool operator!=(Platform const& left, Platform const& right) noexcept;


    constexpr auto to_string(System type) noexcept -> std::string_view;
    constexpr auto to_string(Architecture type) noexcept -> std::string_view;
    constexpr auto to_string(Compiler type) noexcept -> std::string_view;
    constexpr auto to_string(Platform const& type) noexcept -> std::string_view;


    static constexpr Platform platform_uwp_x64_msvc = {
        .name = "uwp-x64-msvc",
        .system = System::UWP,
        .architecture = Architecture::x86_x64,
        .compiler = Compiler::MSVC
    };

    static constexpr Platform platform_windows_x64_msvc = {
        .name = "windows-x64-msvc",
        .system = System::Windows,
        .architecture = Architecture::x86_x64,
        .compiler = Compiler::MSVC
    };

    static constexpr Platform platform_windows_x64_clang = {
        .name = "windows-x64-clang",
        .system = System::Windows,
        .architecture = Architecture::x86_x64,
        .compiler = Compiler::Clang
    };

    static constexpr Platform platform_android_arm64_clang = {
        .name = "android-arm64-clang",
        .system = System::Android,
        .architecture = Architecture::Arm64,
        .compiler = Compiler::Clang
    };

    static constexpr Platform platform_unix_x64_clang = {
        .name = "unix-x64-clang",
        .system = System::Unix,
        .architecture = Architecture::x86_x64,
        .compiler = Compiler::Clang
    };

    static constexpr Platform platform_unix_x64_gcc = {
        .name = "unix-x64-gcc",
        .system = System::Unix,
        .architecture = Architecture::x86_x64,
        .compiler = Compiler::GCC
    };

    static constexpr Platform all_platforms[] = {
        platform_uwp_x64_msvc,
        platform_windows_x64_msvc,
        platform_windows_x64_clang,
        platform_unix_x64_clang,
        platform_unix_x64_gcc,
    };


#if INTPTR_MAX == INT64_MAX
#define ISP_ARCH_BITS 64
#elif INTPTR_MAX == INT32_MAX
#define ISP_ARCH_BITS 32
#else
#error Unknown pointer size or missing size macros!
#endif

#if defined(_WIN64)
#   define ISP_UNIX 0
#   define ISP_WINDOWS 1
#   define ISP_ANDROID 0
#   if defined(__clang__)
#       define ISP_COMPILER_MSVC 0
#       define ISP_COMPILER_CLANG 1
#       define ISP_COMPILER_GCC 0

        static constexpr Platform current_platform = platform_windows_x64_clang;
#   else
#       define ISP_COMPILER_MSVC 1
#       define ISP_COMPILER_CLANG 0
#       define ISP_COMPILER_GCC 0

        static constexpr Platform current_platform = platform_windows_x64_msvc;
#   endif
#elif defined(__ANDROID__)
#   define ISP_UNIX 1
#   define ISP_WINDOWS 0
#   define ISP_ANDROID __ANDROID_API__
#   define ISP_COMPILER_MSVC 0
#   define ISP_COMPILER_CLANG __clang_major__
#   define ISP_COMPILER_GCC 0

    static constexpr Platform current_platform = platform_android_arm64_clang;
#elif __unix__ && !__clang__
#   define ISP_UNIX 1
#   define ISP_WINDOWS 0
#   define ISP_ANDROID 0
#   define ISP_COMPILER_MSVC 0
#   define ISP_COMPILER_CLANG 0
#   define ISP_COMPILER_GCC 1

    static constexpr Platform current_platform = platform_unix_x64_gcc;
#elif __unix__ && __clang__
#   define ISP_UNIX 1
#   define ISP_WINDOWS 0
#   define ISP_ANDROID 0
#   define ISP_COMPILER_MSVC 0
#   define ISP_COMPILER_CLANG __clang_major__
#   define ISP_COMPILER_GCC 0

    static constexpr Platform current_platform = platform_unix_x64_clang;
#else
#   define ISP_UNIX 0
#   define ISP_WINDOWS 0
#   define ISP_ANDROID 0
#   define ISP_COMPILER_MSVC 0
#   define ISP_COMPILER_CLANG 0
#   define ISP_COMPILER_GCC 0

    static_assert(false, "Unknow platform!");
#endif

    constexpr bool operator==(Platform const& left, System right) noexcept
    {
        return left.system == right;
    }

    constexpr bool operator!=(Platform const& left, System right) noexcept
    {
        return !(left == right);
    }

    constexpr bool operator==(Platform const& left, Architecture right) noexcept
    {
        return left.architecture == right;
    }

    constexpr bool operator!=(Platform const& left, Architecture right) noexcept
    {
        return !(left == right);
    }

    constexpr bool operator==(Platform const& left, Compiler right) noexcept
    {
        return left.compiler == right;
    }

    constexpr bool operator!=(Platform const& left, Compiler right) noexcept
    {
        return !(left == right);
    }

    constexpr bool operator==(Platform const& left, Platform const& right) noexcept
    {
        return left.system == right.system
            && left.architecture == right.architecture
            && left.compiler == right.compiler;
    }

    constexpr bool operator!=(Platform const& left, Platform const& right) noexcept
    {
        return !(left == right);
    }


    constexpr auto to_string(System system) noexcept -> std::string_view
    {
        switch (system)
        {
        case ice::build::System::UWP:
            return "universal_windows_platform";
        case ice::build::System::Windows:
            return "windows";
        case ice::build::System::Android:
            return "android";
        case ice::build::System::Unix:
            return "unix";
        default:
            return "<invalid>";
        }
    }

    constexpr auto to_string(Architecture arch) noexcept -> std::string_view
    {
        switch (arch)
        {
        case Architecture::x86:
            return "x86";
        case Architecture::x86_x64:
            return "x64";
        case Architecture::Arm64:
            return "arm64";
        default:
            return "<invalid>";
        }
    }

    constexpr auto to_string(Compiler compiler) noexcept -> std::string_view
    {
        switch (compiler)
        {
        case Compiler::MSVC:
            return "msvc";
        case Compiler::Clang:
            return "clang";
        case Compiler::GCC:
            return "gcc";
        default:
            return "<invalid>";
        }
    }

    constexpr auto to_string(Platform const& platform) noexcept -> std::string_view
    {
        return platform.name;
    }

} // namespace ice::build
