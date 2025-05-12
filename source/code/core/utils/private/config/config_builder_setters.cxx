/// Copyright 2025 - 2025, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#include "config_builder.hxx"

namespace ice
{

    using enum ice::config::detail::KeyType;
    using enum ice::config::detail::ValType;

    using ice::config::detail::ConfigBuilderEntry;
    using ice::config::detail::ConfigBuilderContainer;

    template<>
    auto ConfigBuilderValue::set(bool value) noexcept -> bool&
    {
        ConfigBuilderEntry* const entry = _idx == ice::u32_max
            ? _internal : (static_cast<ConfigBuilderContainer*>(_internal)->_entries._data + _idx);

        // Special case for bool 'sign_bit without any size bits'
        cb_assign_value_type(*_alloc, *entry, CONFIG_VALTYPE_SIGN_BIT);
        entry->data.val_u8 = value;
        return entry->data.val_bool;
    }

    template<>
    auto ConfigBuilderValue::set(ice::u8 value) noexcept -> ice::u8&
    {
        ConfigBuilderEntry* const entry = _idx == ice::u32_max
            ? _internal : (static_cast<ConfigBuilderContainer*>(_internal)->_entries._data + _idx);

        cb_assign_value_type(*_alloc, *entry, CONFIG_VALTYPE_8B_BIT);
        entry->data.val_u8 = value;
        return entry->data.val_u8;
    }

    template<>
    auto ConfigBuilderValue::set(ice::u16 value) noexcept -> ice::u16&
    {
        ConfigBuilderEntry* const entry = _idx == ice::u32_max
            ? _internal : (static_cast<ConfigBuilderContainer*>(_internal)->_entries._data + _idx);

        cb_assign_value_type(*_alloc, *entry, CONFIG_VALTYPE_16B_BIT);
        entry->data.val_u16 = value;
        return entry->data.val_u16;
    }

    template<>
    auto ConfigBuilderValue::set(ice::u32 value) noexcept -> ice::u32&
    {
        ConfigBuilderEntry* const entry = _idx == ice::u32_max
            ? _internal : (static_cast<ConfigBuilderContainer*>(_internal)->_entries._data + _idx);

        cb_assign_value_type(*_alloc, *entry, CONFIG_VALTYPE_32B_BIT);
        entry->data.val_u32 = value;
        return entry->data.val_u32;
    }

    template<>
    auto ConfigBuilderValue::set(ice::u64 value) noexcept -> ice::u64&
    {
        ConfigBuilderEntry* const entry = _idx == ice::u32_max
            ? _internal : (static_cast<ConfigBuilderContainer*>(_internal)->_entries._data + _idx);

        cb_assign_value_type(*_alloc, *entry, CONFIG_VALTYPE_64B_BIT);
        entry->data.val_u64 = value;
        return entry->data.val_u64;
    }

    template<>
    auto ConfigBuilderValue::set(ice::i8 value) noexcept -> ice::i8&
    {
        ConfigBuilderEntry* const entry = _idx == ice::u32_max
            ? _internal : (static_cast<ConfigBuilderContainer*>(_internal)->_entries._data + _idx);

        cb_assign_value_type(*_alloc, *entry, CONFIG_VALTYPE_8B_BIT | CONFIG_VALTYPE_SIGN_BIT);
        entry->data.val_i8 = value;
        return entry->data.val_i8;
    }

    template<>
    auto ConfigBuilderValue::set(ice::i16 value) noexcept -> ice::i16&
    {
        ConfigBuilderEntry* const entry = _idx == ice::u32_max
            ? _internal : (static_cast<ConfigBuilderContainer*>(_internal)->_entries._data + _idx);

        cb_assign_value_type(*_alloc, *entry, CONFIG_VALTYPE_16B_BIT | CONFIG_VALTYPE_SIGN_BIT);
        entry->data.val_i16 = value;
        return entry->data.val_i16;
    }

    template<>
    auto ConfigBuilderValue::set(ice::i32 value) noexcept -> ice::i32&
    {
        ConfigBuilderEntry* const entry = _idx == ice::u32_max
            ? _internal : (static_cast<ConfigBuilderContainer*>(_internal)->_entries._data + _idx);

        cb_assign_value_type(*_alloc, *entry, CONFIG_VALTYPE_32B_BIT | CONFIG_VALTYPE_SIGN_BIT);
        entry->data.val_i32 = value;
        return entry->data.val_i32;
    }

    template<>
    auto ConfigBuilderValue::set(ice::i64 value) noexcept -> ice::i64&
    {
        ConfigBuilderEntry* const entry = _idx == ice::u32_max
            ? _internal : (static_cast<ConfigBuilderContainer*>(_internal)->_entries._data + _idx);

        cb_assign_value_type(*_alloc, *entry, CONFIG_VALTYPE_64B_BIT | CONFIG_VALTYPE_SIGN_BIT);
        entry->data.val_i64 = value;
        return entry->data.val_i64;
    }

    template<>
    auto ConfigBuilderValue::set(ice::f32 value) noexcept -> ice::f32&
    {
        ConfigBuilderEntry* const entry = _idx == ice::u32_max
            ? _internal : (static_cast<ConfigBuilderContainer*>(_internal)->_entries._data + _idx);

        cb_assign_value_type(*_alloc, *entry, CONFIG_VALTYPE_32B_BIT | CONFIG_VALTYPE_SIGN_BIT | CONFIG_VALTYPE_FP_BIT);
        entry->data.val_f32 = value;
        return entry->data.val_f32;
    }

    template<>
    auto ConfigBuilderValue::set(ice::f64 value) noexcept -> ice::f64&
    {
        ConfigBuilderEntry* const entry = _idx == ice::u32_max
            ? _internal : (static_cast<ConfigBuilderContainer*>(_internal)->_entries._data + _idx);

        cb_assign_value_type(*_alloc, *entry, CONFIG_VALTYPE_64B_BIT | CONFIG_VALTYPE_SIGN_BIT | CONFIG_VALTYPE_FP_BIT);
        entry->data.val_f64 = value;
        return entry->data.val_f64;
    }

    auto ConfigBuilderValue::set(ice::String value) noexcept -> ice::HeapVarString<>&
    {
        ConfigBuilderEntry* const entry = _idx == ice::u32_max
            ? _internal : (static_cast<ConfigBuilderContainer*>(_internal)->_entries._data + _idx);

        cb_assign_value_type(*_alloc, *entry, CONFIG_VALTYPE_STRING);
        entry->data.val_varstr = _alloc->create<ice::HeapVarString<>>(*_alloc, value);
        return *entry->data.val_varstr;
    }

} // namespace ice
