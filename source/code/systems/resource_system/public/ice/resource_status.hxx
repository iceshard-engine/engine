/// Copyright 2022 - 2025, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/base.hxx>

namespace ice
{

    enum class ResourceStatus : ice::u32
    {
        Invalid = 0x00'00,
        Available = 0x00'01,
        Loaded = 0x00'02,
        Loading = 0x01'00,
        Unloading = 0x02'00,

        // Enabled flag operators using the 'FlagType' concept.
        None = Invalid,
        All = Invalid | Available | Loaded | Loading | Unloading,
    };

} // namespace ice
