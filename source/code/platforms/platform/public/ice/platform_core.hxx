/// Copyright 2023 - 2024, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/platform.hxx>
#include <ice/input/device_event.hxx>
#include <ice/shard_container.hxx>
#include <ice/task_scheduler.hxx>

namespace ice::platform
{

    //! \brief Provide a HTML5 selector to be used to listen to mouse inputs. (Default "#canvas")
    static constexpr ice::ShardID ShardID_WebAppInputSelectorMouse = "platform/core-webapp/input-selector-mouse`char const*"_shardid;

    //! \brief Provide a HTML5 selector to be used to listen to keyboard inputs. (Default {window}, can't be selected)
    static constexpr ice::ShardID ShardID_WebAppInputSelectorKeyboard = "platform/core-webapp/input-selector-keyboard`char const*"_shardid;

    struct Core
    {
        virtual auto refresh_events() noexcept -> ice::Result = 0;

        virtual auto system_events() noexcept -> ice::ShardContainer const& = 0;
        virtual auto input_events() noexcept -> ice::Span<ice::input::DeviceEvent const> = 0;

        virtual auto graphics_thread() noexcept -> ice::TaskScheduler& = 0;
    };

    template<>
    constexpr inline ice::platform::FeatureFlags Constant_FeatureFlags<ice::platform::Core> = FeatureFlags::Core;

} // namespace ice::platform
