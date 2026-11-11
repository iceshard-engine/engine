#pragma once
#include <ice/asset_category.hxx>

namespace ice::render
{

    static constexpr ice::AssetCategory AssetCategory_Mesh =
        ice::make_asset_category("ice/render_system/mesh");
    static constexpr ice::AssetCategory AssetCategory_Texture2D =
        ice::make_asset_category("ice/render_system/texture-2d");
    static constexpr ice::AssetCategory AssetCategory_Shader =
        ice::make_asset_category("ice/render_system/shader");

} // namespace ice::render