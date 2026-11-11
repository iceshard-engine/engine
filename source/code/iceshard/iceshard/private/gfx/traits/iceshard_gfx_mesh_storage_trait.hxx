#pragma once
#include <ice/asset.hxx>
#include <ice/asset_request.hxx>
#include <ice/asset_storage.hxx>
#include <ice/render/render_buffer.hxx>
#include <ice/world/world_trait.hxx>

namespace ice::gfx
{

    struct GfxMeshEntry
    {
        ice::Asset asset;
        ice::render::Mesh mesh;
        bool released = false;
    };

    class Trait_GfxMeshStorage final
        : public ice::Trait
        , public ice::TraitDevUI
        , public ice::AssetRequestResolver
        , public ice::InterfaceSelectorOf<Trait_GfxMeshStorage, ice::TraitDevUI>
    {
    public: // Implementation of: ice::Trait
        Trait_GfxMeshStorage(ice::Allocator& alloc, ice::TraitContext& ctx) noexcept;
        ~Trait_GfxMeshStorage() noexcept override = default;

        auto activate(ice::WorldStateParams const& params) noexcept -> ice::Task<> override;
        auto deactivate(ice::WorldStateParams const& params) noexcept -> ice::Task<> override;

    public: // Implementation of: ice::AssetRequestResolver
        auto on_asset_released(ice::Asset const& asset) noexcept -> ice::Task<> override;
        auto on_asset_loaded(ice::Asset asset) noexcept -> ice::Task<>;

    public: // Implementation of: ice::TraitDevUI
        auto trait_name() const noexcept -> ice::String override { return "Gfx.MeshStorage"; }
        void build_content() noexcept override;

    private: // Task methods
        auto gfx_update(
            ice::gfx::RenderFrameUpdate const& params,
            ice::AssetStorage& assets
        ) noexcept -> ice::Task<>;

        auto gfx_shutdown(
            ice::render::RenderDevice& device
        ) noexcept -> ice::Task<>;

    private:
        ice::ProxyAllocator _allocator;
        ice::HashMap<GfxMeshEntry> _loaded_meshes;
    };

} // namespace ice
