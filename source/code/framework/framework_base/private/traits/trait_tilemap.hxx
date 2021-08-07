#pragma once
#include <ice/game_physics.hxx>
#include <ice/game_tilemap.hxx>

#include <ice/pod/array.hxx>

namespace ice
{

    class IceWorldTrait_TileMap : public ice::WorldTrait_TileMap
    {
    public:
        IceWorldTrait_TileMap(
            ice::Allocator& alloc,
            ice::WorldTrait_Physics2D& trait_physics
        ) noexcept;
        ~IceWorldTrait_TileMap() noexcept = default;

        void set_tilesize(ice::vec2f tile_size) noexcept override;

        void prepare_tilemap(
            ice::TileMap const& tilemap
        ) noexcept override;

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

    private:
        ice::WorldTrait_Physics2D& _physics;
        ice::vec2f _tilesize;
        ice::pod::Array<ice::TileMap const*> _tilemaps;
    };

} // namespace ice
