/// Copyright 2022 - 2023, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/asset.hxx>
#include <ice/game_render_traits.hxx>
#include <ice/render/render_declarations.hxx>
#include <ice/gfx/gfx_device.hxx>

#include <ice/container/hashmap.hxx>

namespace ice
{

    class AssetRequest;
    struct AssetHandle;

#if 0
    class IceWorldTrait_RenderTextureLoader : public ice::gfx::GfxTrait
    {
    public:
        IceWorldTrait_RenderTextureLoader(
            ice::Allocator& alloc
        ) noexcept;

        void gfx_setup(
            ice::gfx::GfxFrame& gfx_frame,
            ice::gfx::GfxDevice& gfx_device
        ) noexcept override;

        void gfx_update(
            ice::EngineFrame const& engine_frame,
            ice::gfx::GfxFrame& gfx_frame,
            ice::gfx::GfxDevice& gfx_device
        ) noexcept override;

        void gfx_cleanup(
            ice::gfx::GfxFrame& gfx_frame,
            ice::gfx::GfxDevice& gfx_device
        ) noexcept override;

        void on_update(
            ice::EngineFrame& frame,
            ice::EngineRunner& runner,
            ice::WorldPortal& portal
        ) noexcept override;

    private:
        auto load_image(
            ice::AssetRequest* request,
            ice::EngineRunner& runner
        ) noexcept -> ice::Task<>;

        auto unload_image(
            ice::u32 image_idx,
            ice::u64 image_hash,
            ice::EngineRunner& runner
        ) noexcept -> ice::Task<>;

    private:
        ice::Array<ice::render::Image> _images;

        struct Entry
        {
            ice::Asset asset;
            ice::u64 image_hash;
            ice::u32 image_index;
        };

        ice::HashMap<Entry, ContainerLogic::Complex> _tracked_images;
    };


    class WorldTraitArchive;

    void register_trait_render_texture_loader(
        ice::WorldTraitArchive& archive
    ) noexcept;
#endif

} // namespace ice
