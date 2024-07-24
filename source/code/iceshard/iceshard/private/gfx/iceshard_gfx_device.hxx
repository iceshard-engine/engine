/// Copyright 2022 - 2024, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/engine.hxx>
#include <ice/engine_types.hxx>
#include <ice/gfx/gfx_context.hxx>
#include <ice/gfx/gfx_runner.hxx>
#include <ice/render/render_device.hxx>
#include <ice/render/render_driver.hxx>
#include <ice/mem_unique_ptr.hxx>
#include <ice/container/array.hxx>
#include "../iceshard_data_storage.hxx"

namespace ice::gfx
{

    class IceGfxQueueGroup;

    class IceGfxDevice final : public ice::gfx::GfxContext
    {
    public:
        IceGfxDevice(
            ice::Allocator& alloc,
            ice::render::RenderDriver& driver,
            ice::render::RenderSurface& render_surface,
            ice::render::RenderDevice* render_device,
            ice::Array<ice::gfx::IceGfxQueueGroup*> graphics_passes
        ) noexcept;
        ~IceGfxDevice() noexcept override;

        auto device() noexcept -> ice::render::RenderDevice& override;
        auto swapchain() noexcept -> ice::render::RenderSwapchain const& override;

        void recreate_swapchain() noexcept override;

        auto next_frame() noexcept -> ice::u32 override;

        auto queue_group(ice::u32 image_index) noexcept -> ice::gfx::v2::GfxQueueGroup_Temp& override;
        auto queue_group_internal(ice::u32 image_index) noexcept -> ice::gfx::IceGfxQueueGroup&;

        void present(ice::u32 image_index) noexcept override;

        auto data() noexcept -> ice::DataStorage& override { return _data; }
        auto data() const noexcept -> ice::DataStorage const& override { return _data; }

    private:
        ice::Allocator& _allocator;
        ice::render::RenderDriver& _render_driver;
        ice::render::RenderSurface& _render_surface;
        ice::render::RenderDevice* const _render_device;

        ice::render::RenderSwapchain* _render_swapchain;

        ice::Array<ice::gfx::IceGfxQueueGroup*> _graphics_queues;
        ice::IceshardDataStorage _data;
    };

    auto create_graphics_device(
        ice::Allocator& alloc,
        ice::render::RenderDriver& render_driver,
        ice::render::RenderSurface& render_surface,
        ice::Span<ice::gfx::GfxQueueDefinition const> render_queues
    ) noexcept -> ice::UniquePtr<ice::gfx::IceGfxDevice>;

} // namespace ice::gfx
