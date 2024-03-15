/// Copyright 2023 - 2024, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/engine_state_definition.hxx>
#include <ice/world/world_assembly.hxx>
#include <ice/world/world_updater.hxx>
#include <ice/mem_allocator_proxy.hxx>
#include "iceshard_world.hxx"

namespace ice
{

    class IceshardWorldManager final
        : public ice::WorldAssembly
        , public ice::WorldUpdater
        , public ice::EngineStateCommitter
    {
        struct Entry;
    public:
        IceshardWorldManager(
            ice::Allocator& alloc,
            ice::UniquePtr<ice::TraitArchive> trait_archive,
            ice::EngineStateTracker& state_tracker
        ) noexcept;
        ~IceshardWorldManager() noexcept;

        auto create_world(ice::WorldTemplate const& world_template) noexcept -> World* override;
        auto find_world(ice::StringID_Arg name) noexcept -> World* override;
        void destroy_world(ice::StringID_Arg name) noexcept override;

        void query_worlds(ice::Array<ice::StringID>& out_worlds) const noexcept override;

        void query_pending_events(
            ice::ShardContainer& out_events
        ) noexcept override;

        void update(
            ice::TaskContainer& out_tasks,
            ice::Span<ice::Shard const> event_shards
        ) noexcept override;

        auto begin() noexcept { return ice::hashmap::begin(_worlds); }
        auto end() noexcept { return ice::hashmap::end(_worlds); }

    public: // Implementation of: ice::EngineStateCommiter
        bool commit(
            ice::EngineStateTrigger const& trigger,
            ice::Shard trigger_shard,
            ice::ShardContainer& out_shards
        ) noexcept override;

    private:
        ice::ProxyAllocator _allocator;
        ice::UniquePtr<ice::TraitArchive> const _trait_archive;
        ice::EngineStateTracker& _state_tracker;

        struct Entry
        {
            ice::UniquePtr<ice::IceshardWorld> world;
            bool is_active;
        };

    private:
        ice::HashMap<Entry> _worlds;
        ice::ShardContainer _pending_events;
    };

} // namespace ice
