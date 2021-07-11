#pragma once
#include <ice/stringid.hxx>

namespace ice
{

    struct Sprite
    {
        static constexpr ice::StringID Identifier = "ice.component.sprite"_sid;

        ice::StringID material;
    };

    struct SpriteTile
    {
        static constexpr ice::StringID Identifier = "ice.component.sprite-tile"_sid;

        ice::vec2u material_tile{ 0, 0 };
    };

} // namespace ice
