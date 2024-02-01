/// Copyright 2022 - 2023, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/mem_unique_ptr.hxx>
#include <ice/module_register.hxx>
#include <ice/engine_types.hxx>
#include <ice/gfx/gfx_types.hxx>

namespace ice
{

    auto create_engine(
        ice::Allocator& alloc,
        ice::ModuleRegister& registry,
        ice::EngineCreateInfo create_info
    ) noexcept -> ice::UniquePtr<ice::Engine>;

    auto create_engine_runner(
        ice::Allocator& alloc,
        ice::ModuleRegister& registry,
        ice::EngineRunnerCreateInfo const& create_info
    ) noexcept -> ice::UniquePtr<ice::EngineRunner>;

    auto create_gfx_runner(
        ice::Allocator& alloc,
        ice::ModuleRegister& registry,
        ice::gfx::GfxRunnerCreateInfo const& create_info
    ) noexcept -> ice::UniquePtr<ice::gfx::GfxRunner>;

    namespace detail::engine
    {

        using CreateFn = auto (
            ice::Allocator&,
            ice::ModuleRegister&,
            ice::EngineCreateInfo
        ) noexcept -> ice::Engine*;

        using CreateRunnerFn = auto (
            ice::Allocator&,
            ice::ModuleRegister&,
            ice::EngineRunnerCreateInfo const&
        ) noexcept -> ice::EngineRunner*;

        using CreateGfxRunnerFn = auto (
            ice::Allocator&,
            ice::ModuleRegister&,
            ice::gfx::GfxRunnerCreateInfo const&
        ) noexcept -> ice::gfx::GfxRunner*;

        using DestroyFn = void(ice::Engine*) noexcept;
        using DestroyRunnerFn = void(ice::EngineRunner*) noexcept;
        using DestroyGfxRunnerFn = void(ice::gfx::GfxRunner*) noexcept;

        struct EngineAPI
        {
            static constexpr ice::StringID Constant_APIName = "iceshard.engine"_sid;
            static constexpr ice::u32 Constant_APIVersion = 2;

            CreateFn* create_engine_fn;
            DestroyFn* destroy_engine_fn;
            CreateRunnerFn* create_engine_runner_fn;
            DestroyRunnerFn* destroy_engine_runner_fn;
            CreateGfxRunnerFn* create_gfx_runner_fn;
            DestroyGfxRunnerFn* destroy_gfx_runner_fn;
        };

    }

} // namespace ice
