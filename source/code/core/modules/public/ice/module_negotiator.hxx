/// Copyright 2022 - 2024, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/module_concepts.hxx>
#include <ice/module_info.hxx>
#include <ice/module_query.hxx>

namespace ice
{

    using ice::concepts::ModuleNegotiator;

    //! \brief Negotiation API used to register and query modules for their APIs.
    //! \details Allows to register and query APIs independent if the module is dynamically or statically linked.
    struct ModuleNegotiatorAPI
    {
        //! \brief Used to return API pointers into the given array.
        //! \note If the array pointer is null, the size pointer will be set to the required size.
        //! \note If the array is not large enough, the returned elements will be truncated without a specific order.
        using FnModuleSelectAPIs = bool (*)(
            ice::ModuleNegotiatorAPIContext*,
            ice::StringID_Hash api_name,
            ice::u32 api_version,
            ice::ModuleAPI* out_array,
            ice::ucount* inout_array_size
        ) noexcept;

        //! \brief Registers an API selector function for the given API name.
        //! \note A single selector may return multiple versions of the same API.
        using FnModuleRegisterAPI = bool (*)(
            ice::ModuleNegotiatorAPIContext*,
            ice::StringID_Hash api_name,
            ice::FnModuleSelectAPI* fn_api_selector
        ) noexcept;

        FnModuleSelectAPIs fn_select_apis;
        FnModuleRegisterAPI fn_register_api;
    };

    //! \brief Helper type over the ModuleNegotiatorAPI type.
    //! \details Provides a more convenient way to register and query APIs.
    class ModuleNegotiatorBase : public ice::ModuleQuery
    {
    public:
        ModuleNegotiatorBase(
            ice::ModuleNegotiatorAPI* negotiator_api,
            ice::ModuleNegotiatorAPIContext* negotiator_context
        ) noexcept;

        //! \copy ice::ModuleQuery::query_apis
        bool query_apis(
            ice::StringID_Arg api_name,
            ice::u32 api_version,
            ice::ModuleAPI* out_array,
            ice::ucount* inout_array_size
        ) const noexcept override;

        //! \brief Registers an API selector function with the given API name.
        bool register_api(
            ice::StringID_Arg api_name,
            ice::FnModuleSelectAPI* fn_api_selector
        ) const noexcept;

    public:
        ice::ModuleNegotiatorAPI* negotiator_api;
        ice::ModuleNegotiatorAPIContext* negotiator_context;
    };

    template<typename Tag>
    class ModuleNegotiatorTagged final : public ice::ModuleNegotiatorBase
    {
    public:
        using ModuleNegotiatorBase::ModuleNegotiatorBase;
        using ModuleNegotiatorBase::query_apis;
        using ModuleNegotiatorBase::register_api;

        //! \brief Registers an API selector function.
        //! \details The API type is inferred from the first parameter of the given function. If that function parameter
        //!   is a valid API type, the API name, version and priority (if set) are taken from the type.
        template<typename T> requires(ice::concepts::APIType<T>)
        bool register_api(ice::ProcAPIQuickRegisterFunc<T> register_func) const noexcept;
    };

    inline ModuleNegotiatorBase::ModuleNegotiatorBase(
        ice::ModuleNegotiatorAPI* negotiator_api,
        ice::ModuleNegotiatorAPIContext* negotiator_context
    ) noexcept
        : negotiator_api{ negotiator_api }
        , negotiator_context{ negotiator_context }
    { }

    template<typename Tag>
    template<typename T> requires(ice::concepts::APIType<T>)
    inline bool ModuleNegotiatorTagged<Tag>::register_api(ice::ProcAPIQuickRegisterFunc<T> register_func) const noexcept
    {
        static T api_struct = [](ice::ProcAPIQuickRegisterFunc<T> func) noexcept
        {
            T result_struct;
            func(result_struct);
            return result_struct;
        }(register_func);

        return register_api(
            T::Constant_APIName,
            [](ice::StringID_Hash name, ice::u32 version, ice::ModuleAPI* api_ptr) noexcept -> bool
            {
                if (name == T::Constant_APIName && version == T::Constant_APIVersion)
                {
                    api_ptr->api_ptr = &api_struct;
                    api_ptr->version = T::Constant_APIVersion;
                    if constexpr (ice::concepts::APIExplicitPriority<T>)
                    {
                        api_ptr->priority = T::Constant_APIPriority;
                    }
                    else
                    {
                        api_ptr->priority = 100;
                    }
                    return true;
                }
                return false;
            }
        );
    }

} // namespace ice
