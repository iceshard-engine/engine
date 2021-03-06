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

namespace ice
{

    IceshardEngine::IceshardEngine(
        ice::Allocator& alloc,
        ice::AssetSystem& asset_system
    ) noexcept
        : ice::Engine{ }
        , _allocator{ alloc }
        , _asset_system{ asset_system }
    {
    }

    auto IceshardEngine::create_runner(
        ice::render::RenderSurface* render_surface,
        ice::render::RenderDriver* render_driver
    ) noexcept -> ice::UniquePtr<EngineRunner>
    {
        ice::pod::Array<ice::render::QueueFamilyInfo> queue_families{ _allocator };
        render_driver->query_queue_infos(queue_families);

        using ice::render::QueueFlags;
        using ice::render::QueueInfo;
        using ice::render::QueueID;

        constexpr QueueFlags required_flags = QueueFlags::Graphics | QueueFlags::Compute | QueueFlags::Present;

        QueueID queue_id = QueueID::Invalid;
        for (ice::render::QueueFamilyInfo const& queue_family : queue_families)
        {
            if ((queue_family.flags & required_flags) == required_flags)
            {
                queue_id = queue_family.id;
                break;
            }
        }

        if (queue_id != QueueID::Invalid)
        {
            return ice::make_unique<EngineRunner, IceshardEngineRunner>(
                _allocator,
                _allocator,
                render_surface,
                render_driver
            );
        }
        else
        {
            return ice::make_unique_null<ice::EngineRunner>();
        }
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
