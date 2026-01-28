/// Copyright 2023 - 2026, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/native_file.hxx>

namespace ice::app
{

    struct Version
    {
        ice::u16 major;
        ice::u16 minor;
        ice::u32 patch;
        ice::u32 build;
        ice::u64 commit[5];
    };

    auto version() noexcept -> ice::app::Version;
    auto name() noexcept -> ice::String;

    auto location() noexcept -> ice::Path;

    auto directory() noexcept -> ice::Path;

    auto workingdir() noexcept -> ice::Path;

} // namespace ice::app
