/// Copyright 2022 - 2026, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/span.hxx>
#include <ice/clock.hxx>
#include <ice/stringid.hxx>
#include <ice/delegate.hxx>
#include <ice/shard_container.hxx>
#include <ice/input/input_types.hxx>
#include <ice/concept/named_type.hxx>

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

        virtual auto data() noexcept -> ice::DataStorage& = 0;
        virtual auto data() const noexcept -> ice::DataStorage const& = 0;
        // virtual auto data() noexcept -> ice::EngineFrameData& = 0;
        // virtual auto data() const noexcept -> ice::EngineFrameData const& = 0;

        virtual auto shards() noexcept -> ice::ShardContainer& = 0;
        virtual auto shards() const noexcept -> ice::ShardContainer const& = 0;

        template<typename T, typename... Args>
        bool handle_last_shard(
            ice::ShardID shard_id,
            ice::Delegate<void(T const&, Args...)> delegate,
            std::type_identity_t<Args>&&... args
        ) noexcept;

        virtual auto entity_index() noexcept -> ice::ecs::EntityIndex& = 0;
        virtual auto entity_operations() noexcept -> ice::ecs::EntityOperations& = 0;
        virtual auto entity_operations() const noexcept -> ice::ecs::EntityOperations const& = 0;

        virtual auto tasks_container() noexcept -> ice::TaskContainer& = 0;

        template<typename T> requires ice::concepts::NamedDataType<T>
        inline auto get() noexcept -> T&;

        template<typename T> requires ice::concepts::NamedDataType<T>
        inline auto get() const noexcept -> T const&;
    };

    template<typename T, typename... Args>
    bool EngineFrame::handle_last_shard(
        ice::ShardID shard_id,
        ice::Delegate<void(T const&, Args...)> delegate,
        std::type_identity_t<Args>&&... args
    ) noexcept
    {
        T value;
        if (shards().inspect_last(shard_id, value) && delegate.valid())
        {
            delegate(value, ice::forward<Args>(args)...);
            return true;
        }
        return false;
    }

    template<typename T> requires ice::concepts::NamedDataType<T>
    inline auto EngineFrame::get() noexcept -> T&
    {
        return this->data().read_or_store<T>(T::Identifier);
    }

    template<typename T> requires ice::concepts::NamedDataType<T>
    inline auto EngineFrame::get() const noexcept -> T const&
    {
        return this->data().read<T const>(T::Identifier);
    }

} // namespace ice
