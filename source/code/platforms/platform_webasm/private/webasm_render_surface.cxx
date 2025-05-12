/// Copyright 2024 - 2025, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#include "webasm_render_surface.hxx"
#include <ice/render/render_surface.hxx>
#include <ice/error_codes.hxx>

namespace ice::platform::webasm
{

    auto WebASM_RenderSurface::create(ice::platform::RenderSurfaceParams surface_params) noexcept -> ice::Result
    {
        int const result = emscripten_set_canvas_element_size("#canvas", surface_params.dimensions.x, surface_params.dimensions.y);
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

    auto WebASM_RenderSurface::get_dimensions() const noexcept -> ice::vec2u
    {
        ice::vec2i size{};
        emscripten_get_canvas_element_size("#canvas", &size.x, &size.y);
        return ice::vec2u{ size };
    }

    bool WebASM_RenderSurface::get_surface(ice::render::SurfaceInfo& out_surface_info) noexcept
    {
        if (_initialized == false)
        {
            return false;
        }

        out_surface_info.type = ice::render::SurfaceType::HTML5_DOMCanvas;
        out_surface_info.webgpu.selector = "#canvas";
        out_surface_info.webgpu.internal = this;
        return true;
    }

    void WebASM_RenderSurface::destroy() noexcept
    {
    }

} // namespace ice::platform::webasm
