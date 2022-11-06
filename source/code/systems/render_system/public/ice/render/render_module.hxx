/// Copyright 2022 - 2022, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/mem_unique_ptr.hxx>
#include <ice/module_register.hxx>

namespace ice::render
{

    class RenderDriver;

    namespace detail::v1
    {

        using CreateFn = auto (ice::Allocator&) noexcept -> ice::render::RenderDriver*;
        using DestroyFn = void (ice::render::RenderDriver*) noexcept;

        struct RenderAPI
        {
            CreateFn* create_driver_fn;
            DestroyFn* destroy_driver_fn;
        };

    } // namespace api

    auto create_render_driver(
        ice::Allocator& alloc,
        ice::ModuleRegister& registry
    ) noexcept -> ice::UniquePtr<ice::render::RenderDriver>;

} // namespace ice::render
