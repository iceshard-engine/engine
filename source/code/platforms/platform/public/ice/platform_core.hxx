/// Copyright 2023 - 2023, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/platform.hxx>
#include <ice/input/device_event.hxx>
#include <ice/shard_container.hxx>

namespace ice::platform
{

    struct Core
    {
        virtual auto refresh_events() noexcept -> ice::Result = 0;

        virtual auto system_events() noexcept -> ice::ShardContainer const& = 0;
        virtual auto input_events() noexcept -> ice::Span<ice::input::DeviceEvent const> = 0;
    };

    template<>
    constexpr inline ice::platform::FeatureFlags Constant_FeatureFlags<ice::platform::Core> = FeatureFlags::Core;

} // namespace ice::platform
