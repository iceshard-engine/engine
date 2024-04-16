/// Copyright 2024 - 2024, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#include "config_impl.hxx"

ISC_WARNING_PUSH
ISCW_OPERATOR_DEPRECATED_BETWEEN_UNRELATED_ENUMERATIONS(ISCW_OP_DISABLE)
#include <rapidjson/document.h>
#undef assert
ISC_WARNING_POP

namespace ice
{

    namespace detail
    {

        void deserialize_json_array_helper(
            rapidjson::GenericArray<true, rapidjson::Value> const& arr,
            ice::StringID_Arg key,
            ice::MutableConfig& config
        ) noexcept
        {
            ice::u32 const count = arr.Size();

            ice::HostAllocator host_alloc{ };
            ice::ForwardAllocator alloc{ host_alloc, { .bucket_size = 1_KiB } };

            if (arr[0].IsBool())
            {
                ice::Array<bool> final_values{ alloc };
                ice::array::reserve(final_values, count);

                for (auto const& value : arr)
                {
                    if (value.IsBool())
                    {
                        ice::array::push_back(final_values, value.GetBool());
                    }
                }

                ice::config_set_bool_array(config, key, ice::Span<bool>{ final_values });
            }
            else if (arr[0].IsInt())
            {
                ice::Array<ice::i32> final_values{ alloc };
                ice::array::reserve(final_values, count);

                for (auto const& value : arr)
                {
                    if (value.IsInt())
                    {
                        ice::array::push_back(final_values, value.GetInt());
                    }
                }

                ice::config_set_int32_array(config, key, ice::Span<ice::i32>{ final_values });
            }
            else if (arr[0].IsFloat())
            {
                ice::Array<ice::f32> final_values{ alloc };
                ice::array::reserve(final_values, count);

                for (auto const& value : arr)
                {
                    if (value.IsFloat())
                    {
                        ice::array::push_back(final_values, value.GetFloat());
                    }
                }

                ice::config_set_float_array(config, key, ice::Span<ice::f32>{ final_values });
            }
            else if (arr[0].IsDouble())
            {
                ice::Array<ice::f32> final_values{ alloc };
                ice::array::reserve(final_values, count);

                for (auto const& value : arr)
                {
                    if (value.IsDouble())
                    {
                        ice::array::push_back(final_values, static_cast<ice::f32>(value.GetDouble()));
                    }
                }

                ice::config_set_float_array(config, key, ice::Span<ice::f32>{ final_values });
            }
            else if (arr[0].IsString())
            {
                ice::Array<ice::String> final_values{ alloc };
                ice::array::reserve(final_values, count);

                for (auto const& value : arr)
                {
                    if (value.IsString())
                    {
                        ice::array::push_back(final_values, { value.GetString(), value.GetStringLength() });
                    }
                }

                ice::config_set_string_array(config, key, ice::Span<ice::String>{ final_values });
            }
            else
            {
                ICE_ASSERT(false, "Unknown value type in config, key: {}", key);
            }
        }

        void deserialize_json_config_helper(
            rapidjson::Value const& object,
            std::string key,
            ice::MutableConfig& config
        ) noexcept
        {
            for (auto const& entry : object.GetObject())
            {
                if (entry.value.IsObject())
                {
                    deserialize_json_config_helper(entry.value, key + entry.name.GetString() + '.', config);
                }
                else if (entry.value.IsArray())
                {
                    auto values = entry.value.GetArray();
                    ice::u32 const count = values.Size();

                    if (count > 0)
                    {
                        std::string field_key = key.substr(1) + entry.name.GetString();
                        deserialize_json_array_helper(values, ice::stringid(field_key), config);
                    }
                }
                else
                {
                    std::string field_key = key.substr(1) + entry.name.GetString();

                    if (entry.value.IsBool())
                    {
                        ice::config_set_bool(config, ice::stringid(field_key), entry.value.GetBool());
                    }
                    else if (entry.value.IsInt())
                    {
                        ice::config_set_int32(config, ice::stringid(field_key), entry.value.GetInt());
                    }
                    else if (entry.value.IsFloat())
                    {
                        ice::config_set_float(config, ice::stringid(field_key), entry.value.GetFloat());
                    }
                    else if (entry.value.IsDouble())
                    {
                        ice::config_set_float(config, ice::stringid(field_key), static_cast<ice::f32>(entry.value.GetDouble()));
                    }
                    else if (entry.value.IsString())
                    {
                        ice::config_set_string(config, ice::stringid(field_key), { entry.value.GetString(), entry.value.GetStringLength() });
                    }
                    else
                    {
                        ICE_ASSERT(false, "Unknown value type in config!");
                    }
                }
            }
        }

        void deserialize_json_config(ice::Data data, ice::MutableConfig& config) noexcept
        {
            rapidjson::Document doc;
            doc.Parse(reinterpret_cast<char const*>(data.location), data.size.value);
            //IS_ASSERT(doc.IsObject(), "The config is not a valid Json object!");

            deserialize_json_config_helper(doc.GetObject(), ".", config);
        }
    } // namespace detail

} // namespace ice
