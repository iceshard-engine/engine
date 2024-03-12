/// Copyright 2023 - 2023, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#include "iceshard_engine.hxx"
#include <ice/engine_module.hxx>
#include <ice/log_module.hxx>

namespace ice
{

    IceshardEngine::IceshardEngine(
        ice::Allocator& alloc,
        ice::EngineCreateInfo create_info
    ) noexcept
        : _allocator{ alloc }
        , _assets{ ice::move(create_info.assets) }
        , _states{ ice::move(create_info.states) }
        , _worlds{ _allocator, ice::move(create_info.traits), *_states }
        , _entities{ _allocator, 10'000 }
    {
    }

    auto IceshardEngine::assets() noexcept -> ice::AssetStorage&
    {
        return *_assets;
    }

    auto IceshardEngine::worlds() noexcept -> ice::WorldAssembly&
    {
        return _worlds;
    }

    auto IceshardEngine::worlds_updater() noexcept -> ice::WorldUpdater&
    {
        return _worlds;
    }

    auto IceshardEngine::worlds_states() noexcept -> ice::WorldStateTracker&
    {
        return _worlds;
    }

    auto IceshardEngine::entities() noexcept -> ice::ecs::EntityIndex&
    {
        return _entities;
    }

    void IceshardEngine::destroy() noexcept
    {
        _allocator.destroy(this);
    }

    auto create_engine_runner_fn(
        ice::Allocator& alloc,
        ice::ModuleRegister& registry,
        ice::EngineRunnerCreateInfo const& create_info_arg
    ) noexcept -> ice::EngineRunner*;

    void destroy_engine_runner_fn(ice::EngineRunner* runner) noexcept;

    auto create_gfx_runner_fn(
        ice::Allocator& alloc,
        ice::ModuleRegister& registry,
        ice::gfx::GfxRunnerCreateInfo const& create_info
    ) noexcept -> ice::gfx::GfxRunner*;

    void destroy_gfx_runner_fn(ice::gfx::GfxRunner* runner) noexcept;

    auto create_engine_fn(
        ice::Allocator& alloc,
        ice::ModuleRegister& registry,
        ice::EngineCreateInfo create_info
    ) noexcept -> ice::Engine*
    {
        return alloc.create<ice::IceshardEngine>(alloc, ice::move(create_info));
    }

    void destroy_engine_fn(ice::Engine* engine) noexcept
    {
        static_cast<ice::IceshardEngine*>(engine)->destroy();
    }

    bool iceshard_get_api_proc(ice::StringID_Hash api_name, ice::u32 version, void** args) noexcept
    {
        static detail::engine::EngineAPI current_api{
            .create_engine_fn = ice::create_engine_fn,
            .destroy_engine_fn = ice::destroy_engine_fn,
            .create_engine_runner_fn = ice::create_engine_runner_fn,
            .destroy_engine_runner_fn = ice::destroy_engine_runner_fn,
            .create_gfx_runner_fn = ice::create_gfx_runner_fn,
            .destroy_gfx_runner_fn = ice::destroy_gfx_runner_fn,
        };

        if (api_name == "iceshard.engine"_sid && version == 2)
        {
            *args = &current_api;
            return true;
        }
        return false;
    }

} // namespace ice

extern "C"
{

    // #TODO: https://github.com/iceshard-engine/engine/issues/92
#if ISP_WINDOWS
    __declspec(dllexport) void ice_module_load(
        ice::Allocator* alloc,
        ice::ModuleNegotiatorContext* ctx,
        ice::ModuleNegotiator* negotiator
    )
    {
        using ice::operator""_sid_hash;
        ice::initialize_log_module(ctx, negotiator);

        negotiator->fn_register_module(ctx, "iceshard.engine"_sid_hash, ice::iceshard_get_api_proc);
    }

    __declspec(dllexport) void ice_module_unload(
        ice::Allocator* alloc
    )
    {
    }
#endif

}
