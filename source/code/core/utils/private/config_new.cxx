#include "config_new.hxx"

namespace ice
{

    auto config_find_key(ice::Config_v2 const& config, ice::u32 idx) noexcept -> ice::Config_v2::Key const*
    {
        // Can only index into 'none' key types.
        if (config._keys == nullptr || config._keys->type != CONFIG_KEYTYPE_NONE)
        {
            return nullptr;
        }

        Config_v2::Key const* k = config._keys;

        // The first element holds the side of the whole table
        ice::ucount const table_size = (k->offset << 8) | k->size;
        if (idx >= table_size) // OutOfBounds?
        {
            return nullptr;
        }

        return config._keys + idx;
    }

    auto config_find_key(ice::Config_v2 const& config, ice::String key) noexcept -> ice::Config_v2::Key const*
    {
        if (config._keys == nullptr || config._keys->type != CONFIG_KEYTYPE_STRING)
        {
            return nullptr;
        }

        // If this key has multiple parts recursively enter config search
        // TODO: Turn into a loop?
        ice::ucount const key_split_location = ice::string::find_first_of(key, '.');
        if (key_split_location != ice::String_NPos)
        {
            Config_v2 const sub_cfg = config_get(config, ice::string::substr(key, 0, key_split_location));
            return ice::config_find_key(sub_cfg, ice::string::substr(key, key_split_location + 1));
        }

        Config_v2::Key const* key_info = config._keys;
        bool found = key == Config_v2::Key::key_str(*key_info, config._strings);

        while(key_info->next && found == false)
        {
            key_info += 1;
            found = key == Config_v2::Key::key_str(*key_info, config._strings);
        }

        return found ? key_info : nullptr;
    }

    auto config_get(ice::Config_v2 const& config, ice::String key) noexcept -> ice::Config_v2
    {
        Config_v2::Key const* key_info = config_find_key(config, key);
        if (key_info == nullptr || key_info->vtype < CONFIG_VALTYPE_TABLE) // 63 == obj, 62 == array
        {
            return {}; // empty config
        }

        ice::usize_raw const key_idx = key_info - config._keys;

        return {
            // Move both keys and values by their index and relative value where child values are located.
            ._keys = config._keys + config._values[key_idx].internal + key_idx,
            ._values = config._values + config._values[key_idx].internal + key_idx,
            ._strings = config._strings,
            ._data = config._data
        };
    }

    bool config_get_i16(ice::Config_v2 const& config, ice::String key, ice::i16& out_val) noexcept
    {
        Config_v2::Key const* key_info = config_find_key(config, key);

        if (key_info == nullptr || key_info->vtype != (CONFIG_VALTYPE_16B_BIT | CONFIG_VALTYPE_SIGN_BIT)) // Stored directly in `.internal`
        {
            return false;
        }

        out_val = std::bit_cast<ice::i16>((ice::u16) config._values[key_info - config._keys].internal);
        return true;
    }

    bool config_get_u32(ice::Config_v2 const& config, ice::String key, ice::u32& out_val) noexcept
    {
        Config_v2::Key const* key_info = config_find_key(config, key);

        if (key_info == nullptr || key_info->vtype != CONFIG_VALTYPE_32B_BIT) // Stored directly in `.internal`
        {
            return false;
        }

        out_val = config._values[key_info - config._keys].internal;
        return true;
    }

    bool config_get_f64(ice::Config_v2 const& config, ice::String key, ice::f64& out_val) noexcept
    {
        Config_v2::Key const* key_info = config_find_key(config, key);

        if (key_info == nullptr || key_info->vtype != (CONFIG_VALTYPE_64B_BIT | CONFIG_VALTYPE_SIGN_BIT | CONFIG_VALTYPE_FP_BIT)) // Stored directly in `.internal`
        {
            return false;
        }

        ice::u64 const* data = (ice::u64 const*) ice::ptr_add(config._data, { config._values[key_info - config._keys].internal });
        out_val = std::bit_cast<ice::f64>(*data);
        return true;
    }

    bool config_get_str(ice::Config_v2 const& config, ice::String key, ice::String& out_val) noexcept
    {
        Config_v2::Key const* key_info = config_find_key(config, key);

        if (key_info == nullptr || key_info->vtype != CONFIG_VALTYPE_STRING) // Stored directly in `.internal`
        {
            return false;
        }

        char const* const varstr = (char const*) ice::ptr_add(config._data, { config._values[key_info - config._keys].internal });
        ice::u32 bytes = 0;
        ice::u32 const varstr_size = ice::read_varstring_size(varstr, bytes);
        out_val = ice::String{ varstr + bytes, varstr_size };
        return true;
    }

    auto config_load_v2(ice::Data data) noexcept -> ice::Config_v2
    {
        ice::Config_v2::Key const* root = reinterpret_cast<Config_v2::Key const*>(data.location);
        ice::u32 const config_size = ((root->offset << 8) | root->size) + 2; // We need to count root and sentiel values explicitly
        ice::Config_v2::Key const* config_keys = root + 1;

        ice::Config_v2::Value const* root_value = reinterpret_cast<Config_v2::Value const*>(root + config_size);
        ice::Config_v2::Value const* config_values = root_value + 1;

        char const* config_strings = reinterpret_cast<char const*>(root_value + config_size);
        void const* config_data = ice::ptr_add(root, { root_value->internal });

        return { config_keys, config_values, config_strings, config_data };
    }

} // namespace ice
