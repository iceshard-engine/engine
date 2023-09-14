/// Copyright 2023 - 2023, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/platform.hxx>
#include <ice/string/string.hxx>

namespace ice::platform
{

    struct Paths
    {
        virtual auto internal_data() const noexcept -> ice::String = 0;
        virtual auto external_data() const noexcept -> ice::String = 0;
        virtual auto save_data() const noexcept -> ice::String = 0;
    };

    template<>
    constexpr inline ice::platform::FeatureFlags Constant_FeatureFlags<ice::platform::Paths> = FeatureFlags::Paths;

} // namespace ice::platform
