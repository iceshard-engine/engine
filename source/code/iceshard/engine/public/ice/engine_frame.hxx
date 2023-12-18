/// Copyright 2022 - 2023, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/span.hxx>
#include <ice/clock.hxx>
#include <ice/stringid.hxx>
#include <ice/data_storage.hxx>
#include <ice/shard_container.hxx>
#include <ice/input/input_types.hxx>

#include <ice/task.hxx>
#include <ice/task_scheduler.hxx>
#include <ice/ecs/ecs_types.hxx>
#include <ice/engine_types.hxx>

namespace ice
{

    struct DataStorage
    {
        virtual bool set(ice::StringID name, void* value) noexcept = 0;
        virtual bool get(ice::StringID name, void*& value) noexcept = 0;
        virtual bool get(ice::StringID name, void const*& value) const noexcept = 0;
    };

    struct EngineFrameData
    {
        virtual ~EngineFrameData() noexcept = default;

        virtual auto frame() noexcept -> ice::DataStorage& = 0;
        virtual auto runtime() noexcept -> ice::DataStorage& = 0;

        virtual auto frame() const noexcept -> ice::DataStorage const& = 0;
        virtual auto runtime() const noexcept -> ice::DataStorage const& = 0;
        virtual auto persistent() const noexcept -> ice::DataStorage const& = 0;
    };

    struct EngineFrame
    {
        virtual ~EngineFrame() noexcept = default;

        virtual auto allocator() const noexcept -> ice::Allocator& = 0;
        virtual auto index() const noexcept -> ice::u32 = 0;

        virtual auto data() noexcept -> ice::EngineFrameData& = 0;
        virtual auto data() const noexcept -> ice::EngineFrameData const& = 0;

        virtual auto shards() noexcept -> ice::ShardContainer& = 0;
        virtual auto shards() const noexcept -> ice::ShardContainer const& = 0;

        virtual auto entity_operations() noexcept -> ice::ecs::EntityOperations& = 0;
        virtual auto entity_operations() const noexcept -> ice::ecs::EntityOperations const& = 0;
    };

} // namespace ice
