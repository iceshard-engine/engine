/// Copyright 2022 - 2023, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/span.hxx>
#include <ice/stringid.hxx>
#include <ice/data_storage.hxx>
#include <ice/shard_container.hxx>
#include <ice/input/input_types.hxx>

#include <ice/task.hxx>
#include <ice/task_scheduler.hxx>
#include <ice/ecs/ecs_types.hxx>

namespace ice
{

    class EngineFrame
    {
    public:
        virtual ~EngineFrame() noexcept = default;

        virtual auto index() const noexcept -> ice::u32 = 0;

        virtual auto allocator() noexcept -> ice::Allocator& = 0;

        virtual auto input_events() const noexcept -> ice::Span<ice::input::InputEvent const> = 0;;

        virtual auto shards() noexcept -> ice::ShardContainer& = 0;
        virtual auto shards() const noexcept -> ice::ShardContainer const& = 0;

        virtual auto entity_operations() noexcept -> ice::ecs::EntityOperations& = 0;
        virtual auto entity_operations() const noexcept -> ice::ecs::EntityOperations const& = 0;

        virtual auto storage() noexcept -> ice::DataStorage& = 0;
        virtual auto storage() const noexcept -> ice::DataStorage const& = 0;

        virtual auto stage_end() noexcept -> ice::TaskStage<ice::EngineFrame> = 0;
    };

} // namespace ice
