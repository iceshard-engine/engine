#pragma once
#include <string_view>

namespace ice::build
{

    enum class System : uint32_t
    {
        Windows,
        Unix
    };

    enum class Architecture : uint32_t
    {
        x64
    };

    enum class Compiler : uint32_t
    {
        MSVC,
        Clang,
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


    static constexpr Platform platform_windows_x64_msvc = {
        .name = "windows-x64-msvc",
        .system = System::Windows,
        .architecture = Architecture::x64,
        .compiler = Compiler::MSVC
    };

    static constexpr Platform platform_windows_x64_clang = {
        .name = "windows-x64-clang",
        .system = System::Windows,
        .architecture = Architecture::x64,
        .compiler = Compiler::Clang
    };

    static constexpr Platform platform_unix_x64_clang = {
        .name = "unix-x64-clang",
        .system = System::Unix,
        .architecture = Architecture::x64,
        .compiler = Compiler::Clang
    };

    static constexpr Platform all_platforms[] = {
        platform_windows_x64_msvc,
        platform_windows_x64_clang,
        platform_unix_x64_clang,
    };


#if defined(_WIN64)
#   define ISP_UNIX 0
#   define ISP_WINDOWS 1
#if defined(__clang__)
    static constexpr Platform current_platform = platform_windows_x64_clang;
#else
    static constexpr Platform current_platform = platform_windows_x64_msvc;
#endif
#elif __unix__ and __clang__
#   define ISP_UNIX 1
#   define ISP_WINDOWS 0
    static constexpr Platform current_platform = platform_unix_x64_clang;
#else
#   define ISP_UNIX 0
#   define ISP_WINDOWS 0
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
        case ice::build::System::Windows:
            return "windows";
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
        case Architecture::x64:
            return "x64";
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
        default:
            return "<invalid>";
        }
    }

    constexpr auto to_string(Platform const& platform) noexcept -> std::string_view
    {
        return platform.name;
    }

} // namespace ice::build
