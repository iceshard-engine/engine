/// Copyright 2022 - 2023, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#include <ice/render/render_module.hxx>
#include <ice/render/render_driver.hxx>

namespace ice::render
{

    auto create_render_driver(
        ice::Allocator& alloc,
        ice::ModuleRegister& registry
    ) noexcept -> ice::UniquePtr<ice::render::RenderDriver>
    {
        ice::render::detail::v1::RenderAPI render_api;
        if (registry.query_api(render_api))
        {
            ice::render::RenderDriver* driver = render_api.create_driver_fn(alloc);
            return ice::make_unique(render_api.destroy_driver_fn, driver);
        }
        return { };
    }

} // namespace ice::render
