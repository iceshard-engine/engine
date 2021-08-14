#pragma once
#include <ice/game_tilemap.hxx>

namespace ice
{

    auto make_tileset_id(
        ice::u8 tileset_idx,
        ice::u8 tile_flip,
        ice::u16 tile_x,
        ice::u16 tile_y
    ) noexcept -> ice::TileSetID
    {
        ice::u32 result = tileset_idx & 0x0000'000f;
        result <<= 4;
        result |= tile_flip & 0x0000'000f;
        result <<= 12;
        result |= 0x0000'0fff & tile_y;
        result <<= 12;
        result |= 0x0000'0fff & tile_x;
        return static_cast<ice::TileSetID>(result);
    }

} // namespace ice
