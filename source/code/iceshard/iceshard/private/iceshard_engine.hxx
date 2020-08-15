#pragma once
#include "iceshard_service_provider.hxx"
#include "iceshard_execution_instance.hxx"
#include "world/iceshard_world_manager.hxx"

#include <core/memory.hxx>
#include <iceshard/engine.hxx>
#include <input_system/module.hxx>
#include <iceshard/renderer/render_module.hxx>
#include <iceshard/renderer/render_system.hxx>

namespace iceshard
{

    class IceShardEngine final : public Engine
    {
    public:
        IceShardEngine(core::allocator& alloc, resource::ResourceSystem& resources) noexcept;
        ~IceShardEngine() noexcept;

        auto revision() const noexcept -> uint32_t override { return 0x0001; }

        auto asset_system() noexcept -> asset::AssetSystem& override;

        auto input_system() noexcept -> ::input::InputSystem& override;

        auto entity_manager() noexcept -> iceshard::EntityManager& override;

        auto world_manager() noexcept -> iceshard::WorldManager& override;

        auto worker_threads() noexcept -> cppcoro::static_thread_pool& override;

        auto execution_instance() noexcept -> core::memory::unique_pointer<iceshard::ExecutionInstance> override;

        auto services() noexcept -> iceshard::ServiceProvider& override;

    protected:
        auto render_module() noexcept -> iceshard::renderer::RenderModule& override;

    private:
        core::memory::proxy_allocator _allocator;

        resource::ResourceSystem& _resources;

        asset::AssetSystem _asset_system;

        core::memory::unique_pointer<::input::InputModule> _input_module{ nullptr, { core::memory::globals::null_allocator() } };
        core::memory::unique_pointer<iceshard::renderer::RenderModuleInstance> _render_module{ nullptr, { core::memory::globals::null_allocator() } };


        // Managers and service provider
        core::memory::unique_pointer<iceshard::EntityManager> _entity_manager{ nullptr, { core::memory::globals::null_allocator() } };
        core::memory::unique_pointer<iceshard::IceshardServiceProvider> _serivce_provider{ nullptr, { core::memory::globals::null_allocator() } };
        core::memory::unique_pointer<iceshard::IceshardWorldManager> _world_manager{ nullptr, { core::memory::globals::null_allocator() } };

        // Thread pool of the engine.
        cppcoro::static_thread_pool _worker_pool{};

        // The current working execution instance
        ExecutionInstance* _execution_instance_lock = nullptr;
    };

} // namespace iceshard
