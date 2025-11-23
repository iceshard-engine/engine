/// Copyright 2024 - 2025, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/container/hashmap.hxx>
#include <ice/render/render_image.hxx>
#include <ice/world/world_trait.hxx>
#include <ice/gfx/gfx_shards.hxx>
#include <ice/task_scheduler.hxx>
#include <ice/asset_request.hxx>
#include <ice/mem_allocator_proxy.hxx>

namespace ice::gfx
{

    struct GfxImageEntry
    {
        ice::Asset asset;
        ice::render::Image image;
        bool released = false;
    };

    class Trait_GfxImageStorage final
        : public ice::Trait
        , public ice::AssetRequestResolver
    {
    public: // Implementation of: ice::Trait
        Trait_GfxImageStorage(ice::TraitContext& ctx, ice::Allocator& alloc) noexcept;
        ~Trait_GfxImageStorage() noexcept override = default;

    public: // Implementation of: ice::AssetRequestResolver
        auto on_asset_released(ice::Asset const& asset) noexcept -> ice::Task<> override;

    public: // Task methods
        auto gfx_update(
            ice::gfx::RenderFrameUpdate const& update,
            ice::AssetStorage& assets
        ) noexcept -> ice::Task<>;

        auto gfx_shutdown(
            ice::render::RenderDevice& device
        ) noexcept -> ice::Task<>;

    private:
        ice::ProxyAllocator _allocator;
        ice::HashMap<GfxImageEntry> _loaded_images;
    };

} // namespace ice::gfx
