/// Copyright 2022 - 2023, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/stringid.hxx>
#include <ice/mem_unique_ptr.hxx>
#include <ice/render/render_queue.hxx>
#include <ice/render/render_driver.hxx>
#include <ice/gfx/gfx_types.hxx>

namespace ice::gfx
{

    struct GfxDevice
    {
        virtual ~GfxDevice() noexcept = default;

        virtual auto device() noexcept -> ice::render::RenderDevice& = 0;
        virtual auto swapchain() noexcept -> ice::render::RenderSwapchain const& = 0;

        virtual void recreate_swapchain() noexcept = 0;
        virtual auto next_frame() noexcept -> ice::u32 = 0;

        // QueueGroups need a HUUUGE refactor...
        virtual auto queue_group(ice::u32 image_index) noexcept -> ice::gfx::v2::GfxQueueGroup_Temp& = 0;

        virtual void present(ice::u32 image_index) noexcept = 0;
    };

} // namespace ice::gfx
