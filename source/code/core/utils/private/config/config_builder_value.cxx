/// Copyright 2025 - 2025, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#include "config_builder.hxx"

namespace ice
{

    using enum ice::config::detail::KeyType;
    using enum ice::config::detail::ValType;

    using ice::config::detail::ConfigBuilderEntry;
    using ice::config::detail::ConfigBuilderContainer;

    ConfigBuilderValue::ConfigBuilderValue(ice::Allocator* alloc, ice::config::detail::ConfigBuilderEntry* entry, ice::u32 ref) noexcept
        : _alloc{ alloc }
        , _internal{ entry }
        , _idx{ ref }
    {
        if (_internal)
        {
            static_cast<ConfigBuilderContainer*>(_internal)->addref();
        }
    }

    ConfigBuilderValue::~ConfigBuilderValue() noexcept
    {
        if (_internal != nullptr)
        {
            static_cast<ConfigBuilderContainer*>(_internal)->release(*_alloc);
        }
    }

    ConfigBuilderValue::ConfigBuilderValue(ConfigBuilderValue&& other) noexcept
        : _alloc{ std::exchange(other._alloc, nullptr) }
        , _internal{ std::exchange(other._internal, nullptr) }
        , _idx{ std::exchange(other._idx, 0) }
    {
    }

    ConfigBuilderValue::ConfigBuilderValue(ConfigBuilderValue const& other) noexcept
        : _alloc{ other._alloc }
        , _internal{ other._internal }
        , _idx{ other._idx }
    {
        if (_internal != nullptr && _internal->data.val_container != nullptr)
        {
            _internal->data.val_container->addref();
        }
    }

    auto ConfigBuilderValue::operator=(ConfigBuilderValue&& other) noexcept -> ConfigBuilderValue&
    {
        if (this != std::addressof(other))
        {
            if (_internal != nullptr)
            {
                _internal->data.val_container->release(*_alloc);
            }

            _alloc = std::exchange(other._alloc, nullptr);
            _internal = std::exchange(other._internal, nullptr);
            _idx = std::exchange(other._idx, 0);
        }
        return *this;
    }

    auto ConfigBuilderValue::operator=(ConfigBuilderValue const& other) noexcept -> ConfigBuilderValue&
    {
        if (this != std::addressof(other))
        {
            if (_internal != nullptr)
            {
                _internal->data.val_container->release(*_alloc);
            }

            _alloc = other._alloc;
            if (_internal = other._internal; _internal != nullptr && _internal->data.val_container != nullptr)
            {
                _internal->data.val_container->addref();
            }
            _idx = other._idx;
        }
        return *this;
    }

    auto ice::ConfigBuilderValue::operator[](ice::String key) noexcept -> ConfigBuilderValue
    {
        ICE_ASSERT_CORE(ice::string::find_first_of(key, '.') == ice::String_NPos);

        ConfigBuilderEntry* const entry = _idx == ice::u32_max
            ? _internal : (static_cast<ConfigBuilderContainer*>(_internal)->_entries._data + _idx);
        if (entry->vtype == CONFIG_VALTYPE_NONE)
        {
            cb_assign_value_type(*_alloc, *entry, CONFIG_VALTYPE_OBJECT);
        }
        ICE_ASSERT_CORE(entry->vtype >= CONFIG_VALTYPE_OBJECT);

        ConfigBuilderContainer* container = entry->data.val_container;
        ConfigBuilderEntry* value = cb_get_entry(*_alloc, *entry, key, false);

        return ConfigBuilderValue{
            _alloc,
            container,
            (ice::u32)(value - container->_entries._data)
        };
    }

    auto ice::ConfigBuilderValue::operator[](ice::u32 idx) noexcept -> ConfigBuilderValue
    {
        ConfigBuilderEntry* const entry = _idx == ice::u32_max
            ? _internal : (static_cast<ConfigBuilderContainer*>(_internal)->_entries._data + _idx);
        if (entry->vtype == CONFIG_VALTYPE_NONE)
        {
            cb_assign_value_type(*_alloc, *entry, CONFIG_VALTYPE_TABLE);
        }
        ICE_ASSERT_CORE(entry->vtype == CONFIG_VALTYPE_TABLE);

        ConfigBuilderContainer* container = entry->data.val_container;
        ConfigBuilderEntry* value = cb_get_entry(*_alloc, *entry, idx, false);

        return ConfigBuilderValue{
            _alloc,
            container,
            (ice::u32)(value - container->_entries._data)
        };
    }

    void ConfigBuilderValue::reset()
    {
        if (_idx == ice::u32_max)
        {
            reinterpret_cast<ConfigBuilderContainer*>(_internal)->clear();
        }
        else
        {
            cb_clear_value_type(*_alloc, ice::exchange(_internal, nullptr));
        }
    }

} // namespace ice
