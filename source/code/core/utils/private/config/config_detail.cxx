#include "config_detail.hxx"

namespace ice::config::detail
{

    auto get_keyindex(ice::String key) noexcept -> ice::u32
    {
        ice::u32 idx = 0;
        char const* k = ice::string::begin(key);
        while(*k >= '0' && *k <= '9')
        {
            idx *= 10;
            idx += (*k - '0');

            k += 1;
        }
        return idx;
    }

    auto get_subconfig(ice::Config& config, ice::String key) noexcept -> ice::ErrorCode
    {
        ConfigKey const* keyptr = nullptr;
        ConfigValue const* valptr = nullptr;
        ice::config::detail::find(config, key, keyptr, valptr);

        if (keyptr == nullptr) // 63 == obj, 62 == array
        {
            return E_ConfigKeyNotFound;
        }
        else if (keyptr->vtype != ValType::CONFIG_VALTYPE_OBJECT)
        {
            return E_ConfigValueNotAnObject;
        }
        else if (valptr->internal == ice::u32_max)
        {
            // TODO: Might need a different error value.
            return E_ConfigValueTypeMissmatch;
        }

        ice::usize_raw const key_idx = (keyptr - config._keys);
        config._keys += valptr->internal + key_idx;
        config._values += valptr->internal + key_idx;
        return S_Ok;
    }

    auto get_subconfig(ice::Config& config, ice::u32 idx) noexcept -> ice::ErrorCode
    {
        ConfigKey const* keyptr = nullptr;
        ConfigValue const* valptr = nullptr;
        ice::config::detail::find(config, idx, keyptr, valptr);

        if (keyptr == nullptr) // 63 == obj, 62 == array
        {
            return E_ConfigKeyNotFound;
        }
        else if (keyptr->vtype != ValType::CONFIG_VALTYPE_TABLE)
        {
            return E_ConfigValueNotAnTable;
        }
        else if (valptr->internal == ice::u32_max)
        {
            // TODO: Might need a different error value.
            return E_ConfigValueTypeMissmatch;
        }

        ice::usize_raw const key_idx = (keyptr - config._keys);
        config._keys += valptr->internal + key_idx;
        config._values += valptr->internal + key_idx;
        return S_Ok;
    }

    auto keyval(
        ice::Config const& config,
        ice::config::detail::ConfigKey const& key
    ) noexcept -> ice::String
    {
        ICE_ASSERT_CORE(key.type == KeyType::CONFIG_KEYTYPE_STRING);
        return ice::String{ config._strings + key.offset, key.size };
    }

    auto find(
        ice::Config const& config,
        ice::u32 index,
        ice::config::detail::ConfigKey const*& out_key,
        ice::config::detail::ConfigValue const*& out_value
    ) noexcept -> ice::ErrorCode
    {
        using enum KeyType;
        using enum ValType;

        if (config._keys == nullptr)
        {
            return E_ConfigIsInvalid;
        }
        else if (config._keys->type != KeyType::CONFIG_KEYTYPE_NONE)
        {
            return E_ConfigValueNotAnTable;
        }

        // The first element holds the size of the whole table
        ice::ucount const table_size = ( config._keys->offset << 8) |  config._keys->size;
        if (index >= table_size) // OutOfBounds?
        {
            return E_ConfigIndexOutOfBounds;
        }

        out_key = config._keys + index;
        out_value = config._values + index;
        return S_Ok;
    }

    auto find(
        ice::Config const& config,
        ice::String key,
        ice::config::detail::ConfigKey const*& out_key,
        ice::config::detail::ConfigValue const*& out_value
    ) noexcept -> ice::ErrorCode
    {
        using enum KeyType;
        using enum ValType;

        if (config._keys == nullptr)
        {
            return E_ConfigIsInvalid;
        }

        ErrorCode result = S_Ok;
        Config finalcfg = config;

        // If this key has multiple parts recursively enter config search
        ice::ucount key_split_location = ice::string::find_first_of(key, {".|"});
        while (key_split_location != ice::String_NPos && result)
        {
            bool const is_table = finalcfg._keys->type != CONFIG_KEYTYPE_STRING;
            ice::String const keyval = ice::string::substr(key, 0, key_split_location);

            if (is_table)
            {
                result = get_subconfig(finalcfg, get_keyindex(keyval));
            }
            else
            {
                result = get_subconfig(finalcfg, keyval);
            }

            key = ice::string::substr(key, key_split_location + 1);
            key_split_location = ice::string::find_first_of(key, {".|"});
        }

        if (finalcfg._keys->type != CONFIG_KEYTYPE_STRING) [[unlikely]]
        {
            result = ice::config::detail::find(finalcfg, get_keyindex(key), out_key, out_value);
        }
        else
        {
            ConfigKey const* key_info = finalcfg._keys;
            bool found = key == keyval(finalcfg, *key_info);

            while(key_info->next && found == false)
            {
                key_info += 1;
                found = key == keyval(finalcfg, *key_info);
            }

            if (found)
            {
                out_key = key_info;
                out_value = finalcfg._values + (key_info - finalcfg._keys);
            }
            else
            {
                result = E_ConfigKeyNotFound;
            }
        }
        return result;
    }

} // namespace ice::config::detail
