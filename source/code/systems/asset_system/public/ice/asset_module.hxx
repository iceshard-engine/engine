#pragma once
#include <ice/allocator.hxx>
#include <ice/unique_ptr.hxx>
#include <ice/stringid.hxx>

namespace ice
{

    class ModuleRegister;
    class AssetTypeArchive;

    void load_asset_type_definitions(
        ice::Allocator& alloc,
        ice::ModuleRegister const& registry,
        ice::AssetTypeArchive& asset_type_archive
    ) noexcept;

    namespace detail::asset_system::v1
    {


        using RegisterTypesFn = void (ice::AssetTypeArchive&) noexcept;

        struct AssetRegisterTypesAPI
        {
            RegisterTypesFn* register_types_fn;
            // TODO: Unregister or Reload function?
        };

    } // detail::engine::v1

} // namespace ice
