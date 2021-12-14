#include "iceshard_engine.hxx"

#include <ice/input/input_tracker.hxx>

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
#include "iceshard_noop_devui.hxx"

#include "gfx/iceshard_gfx_device.hxx"
#include "gfx/iceshard_gfx_runner.hxx"
#include "gfx/iceshard_gfx_world.hxx"

namespace ice
{

    static ice::IceshardNoopDevUI Global_NoopDevUI;

    IceshardEngine::IceshardEngine(
        ice::Allocator& alloc,
        ice::AssetSystem& asset_system,
        ice::EngineDevUI* devui
    ) noexcept
        : ice::Engine{ }
        , _allocator{ alloc, "engine" }
        , _asset_system{ asset_system }
        , _devui{ devui == nullptr ? &Global_NoopDevUI : devui }
        , _entity_index{ _allocator, 100'000, 500'000 }
        , _world_manager{ _allocator }
    {
    }

    IceshardEngine::~IceshardEngine() noexcept
    {
    }

    auto IceshardEngine::create_runner(
        ice::UniquePtr<ice::input::InputTracker> input_tracker,
        ice::UniquePtr<ice::gfx::GfxRunner> graphics_runner
    ) noexcept -> ice::UniquePtr<ice::EngineRunner>
    {
        return ice::make_unique<EngineRunner, IceshardEngineRunner>(
            _allocator,
            _allocator,
            *this,
            _world_manager,
            ice::move(input_tracker),
            ice::move(graphics_runner)
        );
    }

    auto IceshardEngine::create_graphics_runner(
        ice::render::RenderDriver& render_driver,
        ice::render::RenderSurface& render_surface,
        ice::Span<ice::RenderQueueDefinition const> render_queues
    ) noexcept -> ice::UniquePtr<ice::gfx::GfxRunner>
    {
        ice::UniquePtr<ice::gfx::IceGfxDevice> gfx_device = ice::gfx::create_graphics_device(
            _allocator,
            render_driver,
            render_surface,
            render_queues
        );

        if (gfx_device != nullptr)
        {
            return ice::make_unique<ice::gfx::GfxRunner, ice::gfx::IceGfxRunner>(
                _allocator,
                _allocator,
                ice::move(gfx_device),
                ice::make_unique_null<ice::gfx::IceGfxWorld>()
            );
        }
        else
        {
            return ice::make_unique_null<ice::gfx::GfxRunner>();
        }
    }

    void IceshardEngine::update_runner_graphics(
        ice::EngineRunner& runner,
        ice::UniquePtr<ice::gfx::GfxRunner> graphics_runner
    ) noexcept
    {
        IceshardEngineRunner& iceshard_runner = static_cast<IceshardEngineRunner&>(runner);
        iceshard_runner.set_graphics_runner(ice::move(graphics_runner));
    }

    auto IceshardEngine::entity_index() noexcept -> ice::ecs::EntityIndex&
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

    auto IceshardEngine::developer_ui() noexcept -> ice::EngineDevUI&
    {
        return *_devui;
    }

    auto create_engine_fn(
        ice::Allocator& alloc,
        ice::AssetSystem& asset_system,
        ice::ModuleRegister& registry,
        ice::EngineDevUI* devui
    ) noexcept -> ice::Engine*
    {
        return alloc.make<IceshardEngine>(alloc, asset_system, devui);
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
