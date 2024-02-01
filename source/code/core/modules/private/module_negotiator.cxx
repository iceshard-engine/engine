#include <ice/module_negotiator.hxx>

namespace ice
{

    bool ModuleNegotiator::query_apis(
        ice::StringID_Arg api_name,
        ice::u32 api_version,
        ice::ModuleAPI* api_array,
        ice::u32* array_size
    ) const noexcept
    {
        return negotiator_api->fn_select_apis(negotiator_context, ice::stringid_hash(api_name), api_version, api_array, array_size);
    }

    bool ModuleNegotiator::register_api(ice::StringID_Arg api_name, ice::FnModuleSelectAPI* api_selector) const noexcept
    {
        return negotiator_api->fn_register_api(negotiator_context, ice::stringid_hash(api_name), api_selector);
    }

} // namespace ice