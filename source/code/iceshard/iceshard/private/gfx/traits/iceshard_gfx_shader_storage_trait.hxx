/// Copyright 2024 - 2025, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/container/hashmap.hxx>
#include <ice/render/render_shader.hxx>
#include <ice/world/world_trait.hxx>
#include <ice/gfx/gfx_shards.hxx>
#include <ice/task_scheduler.hxx>
#include <ice/asset_request.hxx>

namespace ice::gfx
{

    struct GfxShaderEntry
    {
        ice::Asset asset;
        ice::render::Shader shader;
        bool released = false;

        bool devui_loaded = false;
    };

    class Trait_GfxShaderStorage final
        : public ice::Trait
        , public ice::TraitDevUI
        , public ice::AssetRequestResolver
        , public ice::InterfaceSelectorOf<Trait_GfxShaderStorage, ice::TraitDevUI>
    {
    public: // Implementation of: ice::Trait
        Trait_GfxShaderStorage(ice::TraitContext& ctx, ice::Allocator& alloc) noexcept;
        ~Trait_GfxShaderStorage() noexcept override = default;

    public: // Implementation of: ice::TraitDevUI
        auto trait_name() const noexcept -> ice::String override { return "Gfx.ShaderStorage"; }
        void build_content() noexcept override;

    public: // Implementation of: ice::AssetRequestResolver
        auto on_asset_released(ice::Asset const& asset) noexcept -> ice::Task<> override;
        auto on_asset_loaded(ice::Asset asset) noexcept -> ice::Task<>;

    public: // Task methods
        auto gfx_update(
            ice::gfx::RenderFrameUpdate const& update,
            ice::AssetStorage& assets
        ) noexcept -> ice::Task<>;

        auto gfx_shutdown(
            ice::render::RenderDevice& device
        ) noexcept -> ice::Task<>;

    private:
        ice::HashMap<GfxShaderEntry> _loaded_shaders;
    };

} // namespace ice::gfx
