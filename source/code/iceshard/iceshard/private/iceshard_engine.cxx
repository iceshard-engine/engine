#include "iceshard_engine.hxx"

#include <ice/engine_runner.hxx>
#include <ice/engine_frame.hxx>

#include <ice/memory/proxy_allocator.hxx>
#include <ice/memory/scratch_allocator.hxx>

#include <ice/module.hxx>
#include <ice/log_module.hxx>
#include <ice/log.hxx>
#include <ice/assert.hxx>
#include <ice/engine.hxx>
#include <ice/engine_module.hxx>

#include "iceshard_runner.hxx"
#include "gfx/iceshard_gfx_device.hxx"

namespace ice
{

    IceshardEngine::IceshardEngine(
        ice::Allocator& alloc,
        ice::AssetSystem& asset_system
    ) noexcept
        : ice::Engine{ }
        , _allocator{ alloc }
        , _asset_system{ asset_system }
        , _entity_index{ _allocator, 100'000, 500'000 }
        , _world_manager{ _allocator }
    {
    }

    auto IceshardEngine::create_runner(
        ice::gfx::GfxDeviceCreateInfo const& gfx_create_info
    ) noexcept -> ice::UniquePtr<EngineRunner>
    {
        ice::UniquePtr<ice::gfx::IceGfxDevice> gfx_device = ice::gfx::create_graphics_device(
            _allocator,
            gfx_create_info
        );

        if (gfx_device != nullptr)
        {
            return ice::make_unique<EngineRunner, IceshardEngineRunner>(
                _allocator,
                _allocator,
                ice::move(gfx_device)
            );
        }
        else
        {
            return ice::make_unique_null<ice::EngineRunner>();
        }
    }

    auto IceshardEngine::entity_index() noexcept -> ice::EntityIndex&
    {
        return _entity_index;
    }

    auto IceshardEngine::asset_system() noexcept -> ice::AssetSystem&
    {
        return _asset_system;
    }

    auto IceshardEngine::world_manager() noexcept -> ice::WorldManager&
    {
        return _world_manager;
    }

    auto create_engine_fn(
        ice::Allocator& alloc,
        ice::AssetSystem& asset_system,
        ice::ModuleRegister& registry
    ) noexcept -> ice::Engine*
    {
        return alloc.make<IceshardEngine>(alloc, asset_system);
    }

    auto destroy_engine_fn(ice::Allocator& alloc, ice::Engine* engine) noexcept
    {
        alloc.destroy(engine);
    }

    static ice::detail::engine::v1::EngineAPI engine_api_v1
    {
        .create_engine_fn = ice::create_engine_fn,
        .destroy_engine_fn = ice::destroy_engine_fn
    };

} // namespace ice

namespace ice
{

    bool iceshard_get_api_proc(ice::StringID_Hash name, ice::u32 version, void** api_ptr) noexcept
    {
        if (name == "iceshard.engine"_sid_hash && version == 1)
        {
            *api_ptr = &ice::engine_api_v1;
            return true;
        }
        return false;
    }

} // namespace ice


extern "C"
{

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

}
