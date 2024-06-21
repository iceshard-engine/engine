/// Copyright 2022 - 2023, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/span.hxx>
#include <ice/clock.hxx>
#include <ice/stringid.hxx>
#include <ice/shard_container.hxx>
#include <ice/input/input_types.hxx>

#include <ice/task.hxx>
#include <ice/task_scheduler.hxx>
#include <ice/task_container.hxx>
#include <ice/engine_types.hxx>
#include <ice/engine_frame_data.hxx>
#include <ice/ecs/ecs_types.hxx>

namespace ice
{

    struct EngineFrame
    {
        virtual ~EngineFrame() noexcept = default;

        virtual auto allocator() const noexcept -> ice::Allocator& = 0;
        virtual auto index() const noexcept -> ice::u32 = 0;

        virtual auto data() noexcept -> ice::EngineFrameData& = 0;
        virtual auto data() const noexcept -> ice::EngineFrameData const& = 0;

        virtual auto shards() noexcept -> ice::ShardContainer& = 0;
        virtual auto shards() const noexcept -> ice::ShardContainer const& = 0;

        virtual auto entity_index() noexcept -> ice::ecs::EntityIndex& = 0;
        virtual auto entity_operations() noexcept -> ice::ecs::EntityOperations& = 0;
        virtual auto entity_operations() const noexcept -> ice::ecs::EntityOperations const& = 0;

        virtual auto tasks_container() noexcept -> ice::TaskContainer& = 0;
    };

} // namespace ice
