#pragma once
#include <ice/base.hxx>
#include <ice/gfx/gfx_subpass.hxx>
#include "../iceshard_gfx_resource_tracker.hxx"

namespace ice::gfx
{

    void create_subpass_resources_primitives(
        ice::render::RenderDevice& render_device,
        ice::gfx::GfxResourceTracker& resource_tracker
    ) noexcept;

    void destroy_subpass_resources_primitives(
        ice::render::RenderDevice& render_device,
        ice::gfx::GfxResourceTracker& resource_tracker
    ) noexcept;

    void create_subpass_resources_imgui(
        ice::render::RenderDevice& render_device,
        ice::gfx::GfxResourceTracker& resource_tracker
    ) noexcept;

    void destroy_subpass_resources_imgui(
        ice::render::RenderDevice& render_device,
        ice::gfx::GfxResourceTracker& resource_tracker
    ) noexcept;

} // namespace ice::gfx
