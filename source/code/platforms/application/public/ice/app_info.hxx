#pragma once
#include <ice/string_types.hxx>

namespace ice::app
{

    struct Version
    {
        ice::u16 major;
        ice::u16 minor;
        ice::u32 patch;
        ice::u64 commit_sha256[4];
    };

    auto version() noexcept -> ice::app::Version;
    auto name() noexcept -> ice::String;

    auto location() noexcept -> ice::String;
    auto directory() noexcept -> ice::String;
    auto workingdir() noexcept -> ice::String;

} // namespace ice::app
