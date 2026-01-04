/// Copyright 2024 - 2026, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/stringid.hxx>
#include <ice/module_info.hxx>
#include <ice/module_concepts.hxx>
#include <ice/container/array.hxx>

namespace ice
{

    struct ModuleQuery
    {
        virtual ~ModuleQuery() noexcept = default;

        //! \brief Queries an API info for the given name and version.
        //! \param[in] api_name The name of the API to query.
        //! \param[in] api_version The version of the API to query.
        //! \param[out] out_api_info The API information if found.
        //! \returns True if the API was found, false otherwise.
        virtual bool query_api(
            ice::StringID_Arg api_name,
            ice::u32 version,
            ice::ModuleAPI& out_api
        ) const noexcept;

        //! \brief Queries all API infos for the given name and version.
        //! \note If the array is not large enough, the returned elements will be truncated without a specific order.
        //!
        //! \param[in] api_name The name of the APIs to query.
        //! \param[in] api_version The version of the APIs to query.
        //! \param[out] out_array The array to write the API infos into.
        //! \param[in,out] inout_array_size The size of the array. Will be set to the required size if the array pointer is null.
        virtual bool query_apis(
            ice::StringID_Arg api_name,
            ice::u32 version,
            ice::ModuleAPI* out_array,
            ice::u32* inout_array_size
        ) const noexcept = 0;

        //! \brief Queries a single API for the given API struct.
        //! \tparam Type The API struct type to query.
        //! \param api_struct The API struct to update if the call was successful.
        //! \return True if the API was found, false otherwise.
        template<typename Type> requires (ice::concepts::APIType<Type>)
        bool query_api(Type& api_struct) const noexcept;

        //! \brief Queries all APIs for the given API struct.
        //! \warning This function will truncate results if more than 10 APIs are found.
        //!
        //! \tparam Type The API struct type to query.
        //! \param[out] out_apis The array to write the API structs into.
        //! \return True if at least one API was found, false otherwise.
        template<typename Type> requires (ice::concepts::APIType<Type>)
        bool query_apis(ice::Array<Type>& out_apis) const noexcept;
    };

    inline bool ModuleQuery::query_api(
        ice::StringID_Arg api_name,
        ice::u32 api_version,
        ice::ModuleAPI& out_api
    ) const noexcept
    {
        ice::u32 idx = 1;
        return query_apis(api_name, api_version, &out_api, &idx);
    }

    template<typename Type> requires (ice::concepts::APIType<Type>)
    inline bool ModuleQuery::query_api(Type& api_struct) const noexcept
    {
        ice::ModuleAPI api_info;
        bool const found = this->query_api(Type::Constant_APIName, Type::Constant_APIVersion, api_info);
        if (found)
        {
            api_struct = *reinterpret_cast<Type*>(api_info.api_ptr);
        }
        return found;
    }

    template<typename Type> requires (ice::concepts::APIType<Type>)
    bool ModuleQuery::query_apis(ice::Array<Type>& out_apis) const noexcept
    {
        ice::u32 num_apis = 0;
        if (query_apis(Type::Constant_APIName, Type::Constant_APIVersion, nullptr, &num_apis))
        {
            // ICE_LOG_IF(
            //     num_apis > 10,
            //     ice::LogSeverity::Warning, ice::LogTag::Engine,
            //     "More than 10 APIs of type {} where queried ({}).\n"
            //     "Use 'query_apis(ice::StringID_Arg, ice::u32, ice::ModuleAPI*, ice::u32*)' instead to avoid truncating results.",
            //     num_apis, Type::Constant_APIName
            // );

            ice::ModuleAPI temp_tab[10];
            query_apis(Type::Constant_APIName, Type::Constant_APIVersion, temp_tab, &num_apis);

            // Fill the array with the found APIs
            ice::array::reserve(out_apis, num_apis);
            for (ice::u32 idx = 0; idx < num_apis; ++idx)
            {
                ice::array::push_back(out_apis, *reinterpret_cast<Type*>(temp_tab[idx].api_ptr));
            }
        }
        return num_apis > 0;
    }

} // namespace ice
