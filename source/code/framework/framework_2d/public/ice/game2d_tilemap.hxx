#pragma once
#include <ice/world/world_trait.hxx>
#include <ice/unique_ptr.hxx>
#include <ice/allocator.hxx>
#include <ice/stringid.hxx>
#include <ice/span.hxx>
#include <ice/task.hxx>

namespace ice
{

    class AssetSystem;

    enum class Entity : ice::u64;

    struct TilePortal
    {
        ice::vec2f size;
        ice::vec2f location;
        ice::StringID target_portal;
    };

    struct TileMaterial
    {
        union
        {
            struct
            {
                ice::u32 group_id : 4;
                ice::u32 material_id_x : 14;
                ice::u32 material_id_y : 14;
            };
            ice::u32 material_id;
        };
    };

    struct TileMaterialGroup
    {
        ice::u32 group_id;
        ice::StringID image_asset;
    };

    struct TileRoom
    {
        ice::StringID name;
        ice::Span<ice::TilePortal const> portals;

        ice::u32 tiles_count;
        ice::Span<ice::vec2f const> tiles_position;
        ice::Span<ice::TileMaterial const> tiles_material;
    };

    struct TileMap
    {
        ice::StringID name;
        ice::Span<ice::TileMaterialGroup const> material_groups;
        ice::Span<ice::TileRoom const> rooms;
    };

    struct TileMapComponent
    {
        static constexpr ice::StringID Identifier = "ice.game2d.tilemap"_sid;

        ice::StringID name;
    };

    class TileMap2DTrait : public ice::WorldTrait
    {
    public:
        virtual ~TileMap2DTrait() noexcept override = default;

        virtual auto load_tilemap(
            ice::StringID_Arg map_name
        ) noexcept -> ice::Task<ice::TileMap const*> = 0;

        virtual void load_tilemap(
            ice::StringID_Arg name,
            ice::TileMap const& tilemap
        ) noexcept = 0;

        virtual auto unload_tilemap(
            ice::StringID_Arg map_name
        ) noexcept -> ice::Task<> = 0;

        virtual auto move_entity(
            ice::Entity entity,
            ice::StringID_Arg target_portal
        ) noexcept -> ice::Task<> = 0;
    };

    auto create_tilemap_trait(
        ice::Allocator& alloc,
        ice::AssetSystem& assets
    ) noexcept -> ice::UniquePtr<TileMap2DTrait>;

} // namespace ice
