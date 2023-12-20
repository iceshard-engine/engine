/// Copyright 2022 - 2023, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#include <ice/engine_module.hxx>
#include <ice/gfx/gfx_runner.hxx>
#include <ice/engine.hxx>
#include <ice/profiler.hxx>

namespace ice
{

    auto create_engine(
        ice::Allocator& alloc,
        ice::ModuleRegister& registry,
        ice::EngineCreateInfo create_info
    ) noexcept -> ice::UniquePtr<ice::Engine>
    {
        IPT_ZONE_SCOPED;
        ice::UniquePtr<ice::Engine> result{ };

        ice::detail::engine::EngineAPI* engine_api;
        if (registry.find_module_api("iceshard.engine"_sid, 2, reinterpret_cast<void**>(&engine_api)))
        {
            ice::Engine* engine = engine_api->create_engine_fn(alloc, registry, ice::move(create_info));
            result = ice::make_unique(engine_api->destroy_engine_fn, engine);
        }

        return result;
    }

    auto create_engine_runner(
        ice::Allocator& alloc,
        ice::ModuleRegister& registry,
        ice::EngineRunnerCreateInfo const& create_info
    ) noexcept -> ice::UniquePtr<ice::EngineRunner>
    {
        IPT_ZONE_SCOPED;
        ice::UniquePtr<ice::EngineRunner> result{ };

        ice::detail::engine::EngineAPI* engine_api;
        if (registry.find_module_api("iceshard.engine"_sid, 2, reinterpret_cast<void**>(&engine_api)))
        {
            ice::EngineRunner* engine = engine_api->create_engine_runner_fn(alloc, registry, create_info);
            result = ice::make_unique(engine_api->destroy_engine_runner_fn, engine);
        }

        return result;
    }

    auto create_gfx_runner(
        ice::Allocator& alloc,
        ice::ModuleRegister& registry,
        ice::gfx::GfxRunnerCreateInfo const& create_info
    ) noexcept -> ice::UniquePtr<ice::gfx::GfxRunner>
    {
        IPT_ZONE_SCOPED;
        ice::UniquePtr<ice::gfx::GfxRunner> result{ };

        ice::detail::engine::EngineAPI* engine_api;
        if (registry.find_module_api("iceshard.engine"_sid, 2, reinterpret_cast<void**>(&engine_api)))
        {
            ice::gfx::GfxRunner* engine = engine_api->create_gfx_runner_fn(alloc, registry, create_info);
            result = ice::make_unique(engine_api->destroy_gfx_runner_fn, engine);
        }

        return result;
    }

} // namespace ice
