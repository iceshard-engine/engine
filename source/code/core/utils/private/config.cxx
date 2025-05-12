/// Copyright 2024 - 2025, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#include "config/config_detail.hxx"

namespace ice
{

    Config::Config() noexcept
        : _keys{ nullptr }
        , _values{ nullptr }
        , _strings{ nullptr }
        , _data{ nullptr }
    {
    }

    Config::Config(Config&& other) noexcept
        : _keys{ ice::exchange(other._keys, nullptr) }
        , _values{ ice::exchange(other._values, nullptr) }
        , _strings{ ice::exchange(other._strings, nullptr) }
        , _data{ ice::exchange(other._data, nullptr) }
    {
    }

    Config::Config(Config const& other) noexcept
        : _keys{ other._keys }
        , _values{ other._values }
        , _strings{ other._strings }
        , _data{ other._data }
    {
    }

    auto Config::operator=(Config&& other) noexcept -> Config&
    {
        if (this != ice::addressof(other))
        {
            _keys = ice::exchange(other._keys, nullptr);
            _values = ice::exchange(other._values, nullptr);
            _strings = ice::exchange(other._strings, nullptr);
            _data = ice::exchange(other._data, nullptr);
        }
        return *this;
    }

    auto Config::operator=(Config const& other) noexcept -> Config&
    {
        if (this != ice::addressof(other))
        {
            _keys = other._keys;
            _values = other._values;
            _strings = other._strings;
            _data = other._data;
        }
        return *this;
    }

    auto config::from_data(ice::Data data) noexcept -> ice::Config
    {
        using Key = ice::config::detail::ConfigKey;
        using Value = ice::config::detail::ConfigValue;

        Key const* root = reinterpret_cast<Key const*>(data.location);
         // We need to count root and sentiel values explicitly
        ice::u32 const config_size = ((root->offset << 8) | root->size) + 2;
        Key const* config_keys = root + 1;

        Value const* root_value = reinterpret_cast<Value const*>(root + config_size);
        Value const* config_values = root_value + 1;

        char const* config_strings = reinterpret_cast<char const*>(root_value + config_size);
        void const* config_data = ice::ptr_add(root, { root_value->internal });

        Config result;
        result._keys = config_keys;
        result._values = config_values;
        result._strings = config_strings;
        result._data = config_data;
        return result;
    }

} // namespace ice
