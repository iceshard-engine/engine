/// Copyright 2022 - 2023, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/mem_unique_ptr.hxx>
#include <ice/module_register.hxx>
#include <ice/engine_types.hxx>

namespace ice
{

    namespace gfx::v2
    {
        struct GfxRunnerCreateInfo;
        struct GfxRunner;
    }

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
            ice::gfx::v2::GfxRunnerCreateInfo const&
        ) noexcept -> ice::gfx::v2::GfxRunner*;

        using DestroyFn = void(ice::Engine*) noexcept;
        using DestroyRunnerFn = void(ice::EngineRunner*) noexcept;
        using DestroyGfxRunnerFn = void(ice::gfx::v2::GfxRunner*) noexcept;

        struct EngineAPI
        {
            CreateFn* create_engine_fn;
            DestroyFn* destroy_engine_fn;
            CreateRunnerFn* create_engine_runner_fn;
            DestroyRunnerFn* destroy_engine_runner_fn;
            CreateGfxRunnerFn* create_gfx_runner_fn;
            DestroyGfxRunnerFn* destroy_gfx_runner_fn;
        };

    }

} // namespace ice
