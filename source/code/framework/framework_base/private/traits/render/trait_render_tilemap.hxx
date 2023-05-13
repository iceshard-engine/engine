/// Copyright 2022 - 2023, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/game_render_traits.hxx>
#include <ice/render/render_declarations.hxx>

#include <ice/resource_meta.hxx>
#include <ice/mem_data.hxx>

namespace ice
{

    class AssetStorage;

    static constexpr ice::String Tilemap_VtxShader = "shaders/game2d/tiled-vtx";
    static constexpr ice::String Tilemap_PixShader = "shaders/game2d/tiled-pix";

    struct Tile;
    struct TileMap;

    struct IceTileLayer_RenderInfo
    {
        bool visible;
        ice::Span<ice::Tile const> tiles;
    };

    struct IceTileMap_RenderInfo
    {
        ice::vec2f tilesize;
        ice::TileMap const* tilemap;
        ice::IceTileLayer_RenderInfo layers[5];
    };

    struct IceTileMap_RenderCache
    {
        ice::u32 image_count;
        ice::render::Image tileset_images[4];
        // TODO: Add image asset tracking
        ice::render::Buffer tileset_properties[4];
        ice::render::ResourceSet tileset_resourceset[1];
    };

    class IceWorldTrait_RenderTilemap : public ice::gfx::GfxTrait, public ice::gfx::GfxContextStage
    {
    public:
        IceWorldTrait_RenderTilemap(
            ice::Allocator& alloc
        ) noexcept;

        void on_activate(
            ice::Engine& engine,
            ice::EngineRunner& runner,
            ice::WorldPortal& portal
        ) noexcept override;

        void on_deactivate(
            ice::Engine& engine,
            ice::EngineRunner& runner,
            ice::WorldPortal& portal
        ) noexcept override;

        void on_update(
            ice::EngineFrame& frame,
            ice::EngineRunner& runner,
            ice::WorldPortal& portal
        ) noexcept override;

        void gfx_setup(
            ice::gfx::GfxFrame& gfx_frame,
            ice::gfx::GfxDevice& gfx_device
        ) noexcept override;

        void gfx_cleanup(
            ice::gfx::GfxFrame& gfx_frame,
            ice::gfx::GfxDevice& gfx_device
        ) noexcept override;

        auto task_gfx_update(
            ice::EngineFrame const& engine_frame,
            ice::gfx::GfxFrame& gfx_frame,
            ice::gfx::GfxDevice& gfx_device
        ) noexcept -> ice::Task<> override;

        void record_commands(
            ice::gfx::GfxContext const& context,
            ice::EngineFrame const& frame,
            ice::render::CommandBuffer command_buffer,
            ice::render::RenderCommands& render_commands
        ) const noexcept override;

    protected:
        void update_resource_tilemap(
            ice::gfx::GfxDevice& gfx_device,
            ice::IceTileMap_RenderInfo const& tilemap_info
        ) noexcept;

        void update_resource_camera(
            ice::gfx::GfxDevice& gfx_device
        ) noexcept;

    private:
        ice::Allocator& _allocator;
        ice::AssetStorage* _asset_system;
        ice::Data _shader_data[2];

        ice::render::ResourceSetLayout _resource_set_layouts[2]{ };
        ice::render::ResourceSet _resource_sets[1];
        ice::render::PipelineLayout _pipeline_layout;
        ice::render::Pipeline _pipeline;
        ice::render::ShaderStageFlags _shader_stages[2];
        ice::render::Shader _shaders[2];

        ice::render::Sampler _sampler;

        ice::StringID _render_camera;
        ice::render::Buffer _render_camera_buffer;
        ice::render::Buffer _tile_flip_buffer;
        ice::render::Buffer _vertex_buffer;
        ice::render::Buffer _instance_buffer;

        ice::TileMap const* _last_tilemap = nullptr;
        ice::HashMap<ice::IceTileMap_RenderCache*> _render_cache;
    };


    class WorldTraitArchive;

    void register_trait_render_tilemap(
        ice::WorldTraitArchive& archive
    ) noexcept;

} // namespace ice
