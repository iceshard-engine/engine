/// Copyright 2025 - 2026, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#include "config_builder_utils.hxx"
#include <ice/heap_string.hxx>

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
        for (;idx < container._entries.size(); ++idx)
        {
            ice::String const entry_key = detail::cb_getkey(container._keystrings, container._entries[idx]);
            if (key == entry_key)
            {
                break;
            }
        }

        ice::config::detail::ConfigBuilderEntry* result;
        if (idx < container._entries.size())
        {
            result = container._entries.begin() + idx;
            if (clean)
            {
                cb_clear_value_type(alloc, result);
            }
        }
        else
        {
            idx = container._entries.size().u32();
            ice::ncount const offset = container._keystrings.size();
            container._keystrings.push_back(key);

            // Add the new entry
            container._entries.push_back(ConfigKey{ 0, CONFIG_KEYTYPE_STRING, CONFIG_VALTYPE_NONE, offset.u32(), key.size().u32()});

            // Return the entry
            result = container._entries.begin() + idx;
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
        if (idx < container._entries.size())
        {
            result = container._entries.begin() + idx;
            if (clean)
            {
                cb_clear_value_type(alloc, result);
            }
        }
        else
        {
            if (idx == ice::u32_max)
            {
                idx = container._entries.size().u32();
            }

            while(container._entries.size() <= idx)
            {
                container._entries.push_back(ConfigKey{ 0, CONFIG_KEYTYPE_NONE, CONFIG_VALTYPE_NONE, 0, 0 });
            }

            // Return the entry
            result = container._entries.begin() + idx;
        }

        return result;
    }

} // namespace ice::config::detail
