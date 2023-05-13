/// Copyright 2022 - 2023, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/stringid.hxx>
#include <ice/mem_unique_ptr.hxx>
#include <ice/render/render_queue.hxx>
#include <ice/render/render_driver.hxx>

namespace ice::gfx
{

    class GfxPass;

    class GfxResourceTracker;

    class GfxDevice
    {
    protected:
        virtual ~GfxDevice() noexcept = default;

    public:
        virtual auto device() noexcept -> ice::render::RenderDevice& = 0;
        virtual auto swapchain() noexcept -> ice::render::RenderSwapchain const& = 0;

        virtual void recreate_swapchain() noexcept = 0;

        virtual auto resource_tracker() noexcept -> ice::gfx::GfxResourceTracker& = 0;
    };

} // namespace ice::gfx
