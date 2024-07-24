/// Copyright 2024 - 2024, Dandielo <dandielo@iceshard.net>
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
        ice::render::Shader shader;
        bool released = false;
    };

    class Trait_GfxShaderStorage final
        : public ice::Trait
        , public ice::AssetRequestResolver
    {
    public: // Implementation of: ice::Trait
        Trait_GfxShaderStorage(ice::Allocator& alloc, ice::TraitContext& ctx) noexcept;
        ~Trait_GfxShaderStorage() noexcept override = default;

    public: // Implementation of: ice::AssetRequestResolver
        auto on_asset_released(ice::Asset const& asset) noexcept -> ice::Task<> override;

    public: // Task methods
        auto gfx_update(ice::gfx::GfxFrameUpdate const& update) noexcept -> ice::Task<>;
        auto gfx_shutdown(ice::gfx::GfxStateChange const& update) noexcept -> ice::Task<>;

    private:
        ice::HashMap<GfxShaderEntry> _loaded_shaders;
    };

} // namespace ice::gfx
