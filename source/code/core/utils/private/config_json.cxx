/// Copyright 2024 - 2026, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#include <ice/config/config_builder.hxx>
#include <ice/config.hxx>

#if ISP_WINDOWS
#undef GetObject
#endif

ISC_WARNING_PUSH
ISCW_OPERATOR_DEPRECATED_BETWEEN_UNRELATED_ENUMERATIONS(ISCW_OP_DISABLE)
#include <rapidjson/document.h>
#undef assert
ISC_WARNING_POP

namespace ice::config
{
    namespace detail
    {

        void deserialize_json_object(
            rapidjson::GenericObject<true, rapidjson::Value> const& obj,
            ice::ConfigBuilderValue& config_table
        ) noexcept;

        void deserialize_json_array(
            rapidjson::GenericArray<true, rapidjson::Value> const& arr,
            ice::ConfigBuilderValue& config_table
        ) noexcept;

        void deserialize_json_value(
            rapidjson::Value const& value,
            ice::ConfigBuilderValue config_table
        ) noexcept
        {
            switch (value.GetType())
            {
            case rapidjson::Type::kTrueType:
                config_table = true;
                break;
            case rapidjson::Type::kFalseType:
                config_table = false;
                break;
            case rapidjson::Type::kNumberType:
                if (value.IsInt()) config_table = value.GetInt();
                else if (value.IsUint()) config_table = value.GetUint();
                else if (value.IsInt64()) config_table = value.GetInt64();
                else if (value.IsUint64()) config_table = value.GetUint64();
                else if (value.IsFloat()) config_table = value.GetFloat();
                else if (value.IsDouble()) config_table = value.GetDouble();
                break;
            case rapidjson::Type::kStringType:
                config_table = ice::String{ value.GetString(), value.GetStringLength() };
                break;
            case rapidjson::Type::kArrayType:
                deserialize_json_array(value.GetArray(), config_table);
                break;
            case rapidjson::Type::kObjectType:
                deserialize_json_object(value.GetObject(), config_table);
                break;
            case rapidjson::Type::kNullType:
            default: break;
            };
        }

        void deserialize_json_object(
            rapidjson::GenericObject<true, rapidjson::Value> const& obj,
            ice::ConfigBuilderValue& config_table
        ) noexcept
        {
            for (auto const& member : obj)
            {
                ice::String const name{ member.name.GetString(), member.name.GetStringLength() };
                deserialize_json_value(
                    member.value,
                    config_table[name]
                );
            }
        }

        void deserialize_json_array(
            rapidjson::GenericArray<true, rapidjson::Value> const& arr,
            ice::ConfigBuilderValue& config_table
        ) noexcept
        {
            ice::u32 const count = arr.Size();
            for (ice::u32 idx = 0; idx < count; ++idx)
            {
                deserialize_json_value(arr[idx], config_table[idx]);
            }
        }

    } // namespace detail

    auto from_json(ice::ConfigBuilder& config_builder, ice::String json) noexcept -> ice::ErrorCode
    {
        rapidjson::Document doc;
        if (doc.Parse(json.begin(), json.size()).HasParseError() == false && doc.IsObject())
        {
            detail::deserialize_json_object(const_cast<rapidjson::Document const&>(doc).GetObject(), config_builder);
            return S_Ok;
        }
        return E_Fail;
    }

    auto config_from_json(ice::Allocator& alloc, ice::String json) noexcept -> ice::Memory
    {
        ice::ConfigBuilder builder{ alloc };
        if (ice::config::from_json(builder, json) == S_Ok)
        {
            return builder.finalize(alloc);
        }
        return {};
    }

    auto from_json(ice::Allocator& alloc, ice::String json, ice::Memory& out_memory) noexcept -> ice::Config
    {
        if (out_memory = config_from_json(alloc, json); out_memory.location != nullptr)
        {
            return ice::config::from_data(ice::data_view(out_memory));
        }
        return {};
    }

} // namespace ice
