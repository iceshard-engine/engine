/// Copyright 2025 - 2025, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#include "config_builder_utils.hxx"
#include <ice/string/heap_string.hxx>

namespace ice::config::detail
{

    auto cb_getkey(
        ice::String stringpack,
        ice::config::detail::ConfigBuilderEntry const& entry
    ) noexcept -> ice::String
    {
        return { stringpack._data + entry.offset, entry.size };
    }

    void cb_clear_value_type(
        ice::Allocator& alloc,
        ice::config::detail::ConfigBuilderEntry* entry
    ) noexcept
    {
        if (entry)
        {
            bool clear_vtype = true;
            if (entry->vtype >= CONFIG_VALTYPE_CONTAINER)
            {
                clear_vtype = entry->data.val_container->release(alloc);
            }
            else if (entry->vtype == CONFIG_VALTYPE_STRING)
            {
                alloc.destroy(entry->data.val_varstr);
            }

            if (clear_vtype)
            {
                entry->vtype = CONFIG_VALTYPE_NONE;
            }
        }
    }

    void cb_assign_value_type(
        ice::Allocator& alloc,
        ice::config::detail::ConfigBuilderEntry& entry,
        ValType vtype
    ) noexcept
    {
        cb_clear_value_type(alloc, &entry);
        entry.vtype = vtype;
        if (entry.vtype >= CONFIG_VALTYPE_CONTAINER)
        {
            entry.data.val_container = alloc.create<ConfigBuilderContainer>(alloc, vtype);
        }
    }

    auto cb_get_entry(
        ice::Allocator& alloc,
        ice::config::detail::ConfigBuilderEntry& entry,
        ice::String key,
        bool clean
    ) noexcept -> ice::config::detail::ConfigBuilderEntry*
    {
        ICE_ASSERT_CORE(entry.vtype == CONFIG_VALTYPE_OBJECT);
        ice::config::detail::ConfigBuilderContainer& container = *entry.data.val_container;

        ice::u32 idx = 0;
        for (;idx < ice::count(container._entries); ++idx)
        {
            ice::String const entry_key = detail::cb_getkey(container._keystrings, container._entries[idx]);
            if (key == entry_key)
            {
                break;
            }
        }

        ice::config::detail::ConfigBuilderEntry* result;
        if (idx < ice::count(container._entries))
        {
            result = ice::array::begin(container._entries) + idx;
            if (clean)
            {
                cb_clear_value_type(alloc, result);
            }
        }
        else
        {
            idx = ice::count(container._entries);
            ice::u32 const offset = ice::string::size(container._keystrings);
            ice::string::push_back(container._keystrings, key);

            // Add the new entry
            ice::array::push_back(container._entries, ConfigKey{ 0, CONFIG_KEYTYPE_STRING, CONFIG_VALTYPE_NONE, offset, ice::size(key) });

            // Return the entry
            result = ice::array::begin(container._entries) + idx;
        }

        return result;
    }

    auto cb_get_entry(
        ice::Allocator& alloc,
        ice::config::detail::ConfigBuilderEntry& entry,
        ice::u32 idx,
        bool clean
    ) noexcept -> ice::config::detail::ConfigBuilderEntry*
    {
        ICE_ASSERT_CORE(entry.vtype == CONFIG_VALTYPE_TABLE);
        ice::config::detail::ConfigBuilderContainer& container = *entry.data.val_container;

        ice::config::detail::ConfigBuilderEntry* result;
        if (idx < ice::count(container._entries))
        {
            result = ice::array::begin(container._entries) + idx;
            if (clean)
            {
                cb_clear_value_type(alloc, result);
            }
        }
        else
        {
            if (idx == ice::u32_max)
            {
                idx = ice::count(container._entries);
            }

            while(ice::count(container._entries) <= idx)
            {
                ice::array::push_back(container._entries, ConfigKey{ 0, CONFIG_KEYTYPE_NONE, CONFIG_VALTYPE_NONE, 0, 0 });
            }

            // Return the entry
            result = ice::array::begin(container._entries) + idx;
        }

        return result;
    }

} // namespace ice::config::detail
