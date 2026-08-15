/// Copyright 2024 - 2026, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#include "webasm_draw_surface.hxx"
#include <ice/render/render_surface.hxx>
#include <ice/error_codes.hxx>

namespace ice::platform::webasm
{

    auto WebASM_DrawSurface::create(ice::platform::DrawSurfaceParams const& surface_params) noexcept -> ice::Result
    {
        int const result = emscripten_set_canvas_element_size(
            "#canvas",
            static_cast<i32>(surface_params.dimensions.x),
            static_cast<i32>(surface_params.dimensions.y)
        );
        _initialized = result == EMSCRIPTEN_RESULT_SUCCESS;

        if (_initialized)
        {
            return S_Success;
        }
        else
        {
            return E_Error;
        }
    }

    auto WebASM_DrawSurface::dimensions() const noexcept -> ice::vec2u
    {
        ice::vec2i size{};
        emscripten_get_canvas_element_size("#canvas", &size.x, &size.y);
        return ice::vec2u{ size };
    }

    auto WebASM_DrawSurface::native_surface() const noexcept -> ice::render::NativeSurface const*
    {
        if (_initialized == false)
        {
            return nullptr;
        }
        return this;
    }

    void WebASM_DrawSurface::destroy() noexcept
    {
    }

    void WebASM_DrawSurface::query_surface_info(ice::render::NativeSurfaceInfo& out_surface_info) const noexcept
    {
        out_surface_info.webgpu.selector = "#canvas";
        // out_surface_info.webgpu.internal = this;
    }

} // namespace ice::platform::webasm
