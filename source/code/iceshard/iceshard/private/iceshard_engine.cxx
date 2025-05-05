/// Copyright 2023 - 2024, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#include "iceshard_engine.hxx"
#include <ice/engine_module.hxx>
#include <ice/devui_context.hxx>
#include <ice/log_module.hxx>
#include <imgui/imgui.h>
#undef assert

namespace ice
{

    IceshardEngine::IceshardEngine(
        ice::Allocator& alloc,
        ice::EngineCreateInfo create_info
    ) noexcept
        : _allocator{ alloc }
        , _assets{ ice::move(create_info.assets) }
        , _states{ ice::move(create_info.states) }
        , _entity_archetypes{ ice::move(create_info.archetypes) }
        , _entity_storage{ _allocator, *_entity_archetypes }
        , _worlds{ _allocator, _entity_storage, ice::move(create_info.traits), *_states }
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

    using EngineAPI = ice::detail::engine::EngineAPI;

    bool iceshard_get_api_proc(ice::StringID_Hash api_name, ice::u32 version, ice::ModuleAPI* api) noexcept
    {
        static EngineAPI current_api{
            .create_engine_fn = ice::create_engine_fn,
            .destroy_engine_fn = ice::destroy_engine_fn,
            .create_engine_runner_fn = ice::create_engine_runner_fn,
            .destroy_engine_runner_fn = ice::destroy_engine_runner_fn,
            .create_gfx_runner_fn = ice::create_gfx_runner_fn,
            .destroy_gfx_runner_fn = ice::destroy_gfx_runner_fn,
        };

        if (api_name == EngineAPI::Constant_APIName && version == EngineAPI::Constant_APIVersion)
        {
            api->api_ptr = &current_api;
            api->version = 2;
            api->priority = 100;
            return true;
        }
        return false;
    }

    void iceshard_get_api_proc(EngineAPI& api) noexcept
    {
        api.create_engine_fn = ice::create_engine_fn;
        api.destroy_engine_fn = ice::destroy_engine_fn;
        api.create_engine_runner_fn = ice::create_engine_runner_fn;
        api.destroy_engine_runner_fn = ice::destroy_engine_runner_fn;
        api.create_gfx_runner_fn = ice::create_gfx_runner_fn;
        api.destroy_gfx_runner_fn = ice::destroy_gfx_runner_fn;
    }

    struct IceShardModule : ice::Module<IceShardModule>
    {
        static bool on_load(ice::Allocator& alloc, ice::ModuleNegotiator auto const& negotiator) noexcept
        {
            ice::devui_setup_context(negotiator);
            ice::LogModule::init(alloc, negotiator);
            return negotiator.register_api(iceshard_get_api_proc);
        }

        IS_WORKAROUND_MODULE_INITIALIZATION(IceShardModule);
    };

} // namespace ice
