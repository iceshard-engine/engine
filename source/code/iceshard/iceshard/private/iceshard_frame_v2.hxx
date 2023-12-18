/// Copyright 2023 - 2023, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/engine_frame.hxx>
#include "iceshard_runner_v2.hxx"

namespace ice
{

    struct IceshardEngineFrame : EngineFrame
    {
        IceshardEngineFrame(
            ice::IceshardFrameData& frame_data
        ) noexcept;
        ~IceshardEngineFrame() noexcept override = default;

        auto allocator() const noexcept -> ice::Allocator& override { return _data._allocator; }
        auto index() const noexcept -> ice::u32 override { return _data._index; }

        auto data() noexcept -> ice::EngineFrameData& override { return _data; }
        auto data() const noexcept -> ice::EngineFrameData const& override { return _data; }

        auto shards() noexcept -> ice::ShardContainer& override { return _shards; }
        auto shards() const noexcept -> ice::ShardContainer const& override { return _shards; }

        auto entity_operations() noexcept -> ice::ecs::EntityOperations& override { return *((ice::ecs::EntityOperations*)0); }
        auto entity_operations() const noexcept -> ice::ecs::EntityOperations const& override { return *((ice::ecs::EntityOperations*)0); }

        ice::IceshardFrameData& _data;
        ice::ShardContainer _shards;
    };

    auto create_iceshard_frame(
        ice::Allocator& alloc, ice::EngineFrameData& frame_data, ice::EngineFrameFactoryUserdata
    ) noexcept -> ice::UniquePtr<ice::EngineFrame>
    {
        return ice::make_unique<ice::IceshardEngineFrame>(alloc, static_cast<ice::IceshardFrameData&>(frame_data));
    }

} // namespace ice::v2
