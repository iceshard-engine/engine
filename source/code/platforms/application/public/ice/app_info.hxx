/// Copyright 2023 - 2025, Dandielo <dandielo@iceshard.net>
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

    auto location() noexcept -> ice::String;

    auto directory() noexcept -> ice::String;

    auto workingdir() noexcept -> ice::String;

} // namespace ice::app
