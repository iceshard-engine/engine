/// Copyright 2025 - 2025, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#include <ice/input_action.hxx>

#include <ice/module.hxx>
#include <ice/module_register.hxx>
#include <ice/resource_compiler_api.hxx>
#include <ice/asset_module.hxx>
#include <ice/asset_category_archive.hxx>
#include <ice/log_module.hxx>

namespace ice
{

    void asset_category_shader_definition(
        ice::AssetCategoryArchive& asset_category_archive,
        ice::ModuleQuery const& module_query
    ) noexcept
    {
        static ice::String ext[]{ ".ias" };
        static ice::AssetCategoryDefinition const definition{
            .resource_extensions = ext
        };
        asset_category_archive.register_category(ice::AssetCategory_InputActionsScript, definition);
    }

    struct InputActionsModule : public ice::Module<InputActionsModule>
    {

#if 0
        auto ias_compiler_supported_resources(
            ice::Span<ice::Shard const> params
        ) noexcept -> ice::Span<ice::String const>
        {
            static ice::String ext[]{ ".ias" };
            return ext;
        }

        static void v1_compiler_api(ice::api::resource_compiler::v1::ResourceCompilerAPI& api) noexcept
        {
            api.id_category = "ice/ias-script-resource"_sid;
            api.fn_supported_resources = ias_compiler_supported_resources;
        }
#endif

        static void v1_archive_api(ice::detail::asset_system::v1::AssetArchiveAPI& api) noexcept
        {
            api.fn_register_categories = asset_category_shader_definition;
        }

        static bool on_load(ice::Allocator& alloc, ice::ModuleNegotiator auto& negotiator) noexcept
        {
            // Since we are on the 'system' layer, we can be part of multiple dynamic libararies, and prefere to not be loaded from them.
            if (negotiator.from_app())
            {
#if 0 // Currently we don't really support compiling for InputAction scripts.
                negotiator.register_api(v1_compiler_api);
#endif
                negotiator.register_api(v1_archive_api);
            }
            return true;
        }

        ICE_WORKAROUND_MODULE_INITIALIZATION(InputActionsModule);
    };

} // namespace ice
