#pragma once
#include <ice/game_tilemap.hxx>

namespace ice
{

    auto make_tileid(
        ice::u8 tileset_idx,
        ice::u8 tile_flip,
        ice::u16 tile_x,
        ice::u16 tile_y
    ) noexcept -> ice::TileID
    {
        ice::u32 result = tileset_idx & 0x0000'000f;
        result <<= 4;
        result |= tile_flip & 0x0000'000f;
        result <<= 12;
        result |= 0x0000'0fff & tile_y;
        result <<= 12;
        result |= 0x0000'0fff & tile_x;
        return static_cast<ice::TileID>(result);
    }

} // namespace ice
