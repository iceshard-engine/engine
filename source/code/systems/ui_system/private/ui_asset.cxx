#include <ice/ui_asset.hxx>

namespace ice::ui
{

    void register_ui_asset(ice::AssetTypeArchive& type_archive) noexcept
    {
        static ice::Utf8String asset_extensions[]{
            u8"isui"
        };

        static ice::AssetTypeDefinition asset_definition
        {
            .resource_extensions = asset_extensions
        };

        type_archive.register_type(ice::ui::AssetType_UIPage, asset_definition);
    }

} // namespace ice::ui
