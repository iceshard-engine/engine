/// Copyright 2022 - 2025, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/stringid.hxx>
#include <ice/mem_unique_ptr.hxx>
#include <ice/render/render_queue.hxx>
#include <ice/render/render_driver.hxx>
#include <ice/gfx/gfx_types.hxx>

namespace ice::gfx
{

    //! \brief An object providing access to low-level render objects
    struct GfxContext
    {
        virtual ~GfxContext() noexcept = default;

        //! \returns Access to the render device object representing the GPU.
        virtual auto device() noexcept -> ice::render::RenderDevice& = 0;

        //! \returns Swapchain object currently in use.
        //! \note The object may be recreated in certain events, so it's not advised to hold a reference to it.
        virtual auto swapchain() const noexcept -> ice::render::RenderSwapchain const& = 0;

        //! \brief Recreates the swapchain object using the current render surface properties.
        //! \note This should only be called after all GPU was finished and before submitting new work.
        //! \see Accesses values from `ice::platform::RenderSurface`
        virtual void recreate_swapchain() noexcept = 0;

        virtual auto data() noexcept -> ice::DataStorage& = 0;
        virtual auto data() const noexcept -> ice::DataStorage const& = 0;

    public:
        //! \brief Advances the GPU state the next swapchain image.
        //! \warning This should not be called from a game, the engine is responsible for handling this.
        virtual auto next_frame() noexcept -> ice::u32 = 0;

        //! \brief Presents the current swapchain immage to the render surface.
        //! \warning This should not be called from a game, the engine is responsible for handling this.
        virtual void present(ice::u32 image_index) noexcept = 0;

        //! \warning Old deprecated API, do not use.
        virtual auto queue_group(ice::u32 image_index) noexcept -> ice::gfx::v2::GfxQueueGroup_Temp & = 0;
    };

} // namespace ice::gfx
