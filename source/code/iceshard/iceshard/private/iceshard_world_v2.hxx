#pragma once
#include <ice/world/world.hxx>
#include <ice/world/world_trait.hxx>
#include <ice/world/world_manager.hxx>
#include <ice/world/world_trait_archive.hxx>
#include <ice/container/array.hxx>
#include <ice/engine_runner.hxx>
#include <ice/sync_manual_events.hxx>
#include <ice/clock.hxx>

namespace ice
{

    struct IceshardEventHandler
    {
        ice::ShardID event_id;
        ice::Trait* trait;
        ice::TraitIndirectTaskFn event_handler;
        void* userdata;
    };

    static_assert(sizeof(IceshardEventHandler) == 32);

    class IceshardTraitTaskLauncher final : public ice::TraitTaskLauncher
    {
    public:
        IceshardTraitTaskLauncher(
            ice::Trait* trait,
            ice::HashMap<ice::IceshardEventHandler>& handlers
        ) noexcept;

        void bind(ice::TraitIndirectTaskFn func) noexcept override { bind(func, ice::ShardID_FrameUpdate); }
        void bind(ice::TraitIndirectTaskFn func, ice::ShardID event) noexcept override;

    private:
        ice::Trait* _trait;
        ice::HashMap<ice::IceshardEventHandler>& _handlers;
    };

    class IceshardTasksLauncher
    {
    public:
        IceshardTasksLauncher(ice::Allocator& alloc) noexcept;

        void execute(ice::Array<ice::Task<>, ice::ContainerLogic::Complex>& out_tasks, ice::Shard shard) noexcept;
        void execute(ice::Array<ice::Task<>, ice::ContainerLogic::Complex>& out_tasks, ice::ShardContainer const& shards) noexcept;

        //bool finished() noexcept;

        auto trait_launcher(ice::Trait* trait) noexcept -> ice::IceshardTraitTaskLauncher;

    private:
        ice::HashMap<IceshardEventHandler> _handlers;
        //ice::ManualResetBarrier _barrier;
    };

    class IceshardWorld final : public ice::World
    {
    public:
        IceshardWorld(
            ice::Allocator& alloc,
            ice::Array<ice::UniquePtr<ice::Trait>, ice::ContainerLogic::Complex> traits
        ) noexcept;

        auto trait(ice::StringID_Arg trait_identifier) noexcept -> ice::Trait* override { return nullptr; }
        auto trait(ice::StringID_Arg trait_identifier) const noexcept -> ice::Trait const* override { return nullptr; }
        auto trait_storage(ice::Trait* trait) noexcept -> ice::DataStorage* override { return nullptr; }
        auto trait_storage(ice::Trait const* trait) const noexcept -> ice::DataStorage const* override { return nullptr; }

        auto task_launcher() noexcept -> ice::IceshardTasksLauncher& { return _tasks_launcher; }

        auto activate(ice::EngineWorldUpdate const& update) noexcept -> ice::Task<>;
        auto deactivate(ice::EngineWorldUpdate const& update) noexcept -> ice::Task<>;

    private:
        ice::IceshardTasksLauncher _tasks_launcher;
        ice::Array<ice::UniquePtr<ice::Trait>, ice::ContainerLogic::Complex> _traits;
    };

    class IceshardWorldManager : public ice::WorldAssembly, public ice::WorldUpdater
    {
    public:
        IceshardWorldManager(
            ice::Allocator& alloc,
            ice::UniquePtr<ice::TraitArchive> trait_archive
        ) noexcept;

        auto create_world(ice::WorldTemplate const& world_template) noexcept -> World* override;
        auto find_world(ice::StringID_Arg name) noexcept -> World* override;
        void destroy_world(ice::StringID_Arg name) noexcept override;

        void activate(
            ice::StringID_Arg world_name,
            ice::EngineFrame& frame,
            ice::EngineWorldUpdate const& world_update,
            ice::Array<ice::Task<>, ContainerLogic::Complex>& out_tasks
        ) noexcept;

        void deactivate(
            ice::StringID_Arg world_name,
            ice::EngineFrame& frame,
            ice::EngineWorldUpdate const& world_update,
            ice::Array<ice::Task<>, ContainerLogic::Complex>& out_tasks
        ) noexcept;

        void update(
            ice::EngineFrame& frame,
            ice::EngineWorldUpdate const& world_update,
            ice::Array<ice::Task<>, ContainerLogic::Complex>& out_tasks
        ) noexcept override;

        void force_update(
            ice::StringID_Arg world_name,
            ice::Shard shard,
            ice::Array<ice::Task<>, ContainerLogic::Complex>& out_tasks
        ) noexcept override;

        void update(
            ice::Shard shard,
            ice::Array<ice::Task<>, ContainerLogic::Complex>& out_tasks
        ) noexcept override;

        void update(
            ice::ShardContainer const& shards,
            ice::Array<ice::Task<>, ContainerLogic::Complex>& out_tasks
        ) noexcept override;

        auto begin() noexcept { return ice::hashmap::begin(_worlds); }
        auto end() noexcept { return ice::hashmap::end(_worlds); }

    private:
        ice::Allocator& _allocator;
        ice::UniquePtr<ice::TraitArchive> const _trait_archive;

        struct Entry
        {
            ice::UniquePtr<ice::IceshardWorld> world;
            bool is_active;
        };

        ice::HashMap<Entry, ice::ContainerLogic::Complex> _worlds;
    };

} // namespace ice::v2
