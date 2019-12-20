#pragma once
#include "iceshard_service_provider.hxx"
#include "world/iceshard_world_manager.hxx"
#include "frame.hxx"

#include <core/memory.hxx>
#include <iceshard/engine.hxx>
#include <input_system/module.hxx>
#include <render_system/render_module.hxx>

namespace iceshard
{

    class IceShardEngine final : public Engine
    {
    public:
        IceShardEngine(core::allocator& alloc, resource::ResourceSystem& resources) noexcept;
        ~IceShardEngine() noexcept;

        auto revision() const noexcept -> uint32_t override { return 0x0001; }

        auto asset_system() noexcept -> asset::AssetSystem* override;

        auto input_system() noexcept -> input::InputSystem* override;

        auto entity_manager() noexcept -> iceshard::EntityManager* override;

        auto world_manager() noexcept -> iceshard::WorldManager* override;

        auto previous_frame() const noexcept -> const Frame& override;

        auto current_frame() noexcept -> Frame& override;

        void next_frame() noexcept override;

        auto worker_threads() noexcept -> cppcoro::static_thread_pool& override;

        void add_task(cppcoro::task<> task) noexcept override;

    private:
        auto render_system(render::api::RenderInterface*& render_api) noexcept -> render::RenderSystem* override;

    private:
        core::memory::proxy_allocator _allocator;

        resource::ResourceSystem& _resources;

        asset::AssetSystem _asset_system;

        core::memory::unique_pointer<input::InputModule> _input_module{ nullptr, { core::memory::globals::null_allocator() } };
        core::memory::unique_pointer<render::RenderSystemModule> _render_module{ nullptr, { core::memory::globals::null_allocator() } };

        // Managers and service provider
        core::memory::unique_pointer<iceshard::EntityManager> _entity_manager{ nullptr, { core::memory::globals::null_allocator() } };
        core::memory::unique_pointer<iceshard::IceshardServiceProvider> _serivce_provider{ nullptr, { core::memory::globals::null_allocator() } };
        core::memory::unique_pointer<iceshard::IceshardWorldManager> _world_manager{ nullptr, { core::memory::globals::null_allocator() } };

        // Thread pool of the engine.
        cppcoro::static_thread_pool _worker_pool{};

        // Tasks to be run this frame.
        size_t _task_list_index = 0;
        std::vector<cppcoro::task<>> _frame_tasks[2]{ {}, {} };

        std::atomic<std::vector<cppcoro::task<>>*> _mutable_task_list = nullptr;

        // Frame allocators.
        uint32_t _next_free_allocator = 0;

        core::memory::scratch_allocator _frame_allocator;
        core::memory::scratch_allocator _frame_data_allocator[2];

        // Frames.
        core::memory::unique_pointer<MemoryFrame> _previous_frame;
        core::memory::unique_pointer<MemoryFrame> _current_frame;
    };

} // namespace iceshard
