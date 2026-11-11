#pragma once
#include <ice/asset_category_archive.hxx>

namespace ice
{

    void asset_category_mesh_definition(
        ice::AssetCategoryArchive& asset_category_archive,
        ice::ModuleQuery const& module_query
    ) noexcept;

} // namespace ice