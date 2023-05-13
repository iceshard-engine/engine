/// Copyright 2022 - 2023, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/mem_unique_ptr.hxx>
#include <ice/shard_container.hxx>
#include <ice/input/device_event_queue.hxx>

namespace ice::platform
{

    class App
    {
    public:
        virtual ~App() noexcept = default;

        virtual void handle_inputs(ice::input::DeviceEventQueue const& device_events) noexcept = 0;
        virtual void update(ice::ShardContainer const& shards) noexcept = 0;

        virtual bool requested_exit() const noexcept = 0;
    };

    class Container
    {
    public:
        virtual ~Container() noexcept = default;

        virtual auto step() noexcept -> ice::u32 = 0;

        [[deprecated("Executing the whole loop in a single call is discouraged! Call 'step' whenever you want an updated by yourself!")]]
        virtual auto run() noexcept -> ice::i32 = 0;
    };

    auto create_app_container(
        ice::Allocator& alloc,
        ice::UniquePtr<ice::platform::App> app
    ) noexcept -> ice::UniquePtr<ice::platform::Container>;

} // namespace ice::platform
