/// Copyright 2025 - 2026, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#include <ice/config/config_builder.hxx>
#include <ice/container/hashmap.hxx>
#include <ice/heap_string.hxx>
#include <ice/heap_varstring.hxx>

#include "config_builder.hxx"

namespace ice
{

    using enum ice::config::detail::KeyType;
    using enum ice::config::detail::ValType;

    using ice::config::detail::ConfigBuilderEntry;
    using ice::config::detail::ConfigBuilderContainer;

    ConfigBuilder::ConfigBuilder(ice::Allocator &alloc) noexcept
        : ConfigBuilderValue{
            ice::addressof(alloc),
            alloc.create<ConfigBuilderContainer>(alloc, CONFIG_VALTYPE_OBJECT),
            ice::u32_max
        }
    {
        _internal->data.val_container = static_cast<ConfigBuilderContainer*>(_internal);
    }

    ice::ConfigBuilder::~ConfigBuilder() noexcept
    {
        cb_clear_value_type(*_alloc, _internal);
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

    struct CBKeyString
    {
        ice::String value;
        ice::u32 index;
    };

    struct CBDataBuffer
    {
        ice::Memory memory;
        ice::u32 offset;
        ice::u32 offset_strings;
    };

    auto cb_find_keystr_idx(ice::HashMap<CBKeyString> const& keystrings, ice::String keystr) noexcept -> ice::u32
    {
        ice::u64 const keystr_hash = ice::hash(keystr);
        auto it = ice::multi_hashmap::find_first(keystrings, keystr_hash);
        while(it != nullptr && it.value().value != keystr)
        {
            it = ice::multi_hashmap::find_next(keystrings, it);
        }

        return it == nullptr ? ice::hashmap::count(keystrings) : it.value().index;
    }

    auto cb_calculate_key_size(
        ice::HashMap<CBKeyString>& keystrings,
        ice::config::detail::ConfigBuilderContainer const& config,
        ice::config::detail::ConfigBuilderEntry const* entry
    ) noexcept -> ice::usize
    {
        ice::usize result = 8_B; // Key + Value
        if (entry->type == CONFIG_KEYTYPE_STRING)
        {
            // Find the keystring in the hashmap
            ice::String const keystr = cb_getkey(config._keystrings, *entry);
            ice::u32 const keystr_idx = cb_find_keystr_idx(keystrings, keystr);

            // If it's not there, store a new entry and give it a new index.
            //   This allows us to reuse duplicate key names
            if (keystr_idx == ice::hashmap::count(keystrings))
            {
                ice::multi_hashmap::insert(keystrings, ice::hash(keystr), { keystr, keystr_idx });
                result += { entry->size }; // Increase the size required
            }
        }
        return result;
    }

    auto cb_calculate_final_size(
        ice::HashMap<CBKeyString>& keystrings,
        ice::config::detail::ConfigBuilderContainer const& config,
        ice::usize& out_data_size,
        ice::u32& out_count
    ) noexcept -> ice::usize
    {
        ice::usize result = 0_B;
        ConfigBuilderEntry const* entry = config._entries.begin();
        ConfigBuilderEntry const* const entry_end = config._entries.end();

        if (entry == entry_end)
        {
            return result;
        }

        do
        {
            result += cb_calculate_key_size(keystrings, config, entry); // key + val

            if (entry->vtype >= CONFIG_VALTYPE_CONTAINER)
            {
                // We go through all keys one by one, because we can have holes in the builder.
                result += cb_calculate_final_size(
                    keystrings,
                    *entry->data.val_container,
                    out_data_size,
                    out_count
                );
            }
            else if (entry->vtype >= CONFIG_VALTYPE_USER)
            {
                ICE_ASSERT_CORE(false);
            }
            else if (entry->vtype == CONFIG_VALTYPE_STRING)
            {
                ice::usize bytes = 0_B;
                ice::ncount const size = ice::varstring::read_size(entry->data.val_varstr->_data, bytes);
                out_data_size += bytes + size.bytes() + 1_B; // +1 for '\0'
            }
            else if (entry->vtype >= CONFIG_VALTYPE_LARGE)
            {
                out_data_size += 8_B;
            }

            entry += 1;
            out_count += 1;
        }
        while(entry != entry_end);

        // Config_v2::Key const* final_key = config._keys + config._count;
        return result;
    }

    auto cb_calculate_final_size(
        ice::HashMap<CBKeyString>& keystrings,
        ice::config::detail::ConfigBuilderContainer const& config,
        ice::u32& out_count
    ) noexcept -> ice::usize
    {
        ice::usize data_size = 0_B;
        // We also need to add 16 bytes for both sentiel and root key-value pairs (2 * 2 * 4)
        ice::usize result = 16_B + cb_calculate_final_size(keystrings, config, data_size, out_count);

        if (data_size > 0_B)
        {
            result = ice::align_to(result, ice::ualign::b_8).value;
            result += data_size;
        }

        return result;
    }

    auto cb_finalize_store_keysvalues(
        ice::HashMap<CBKeyString>& keystrings,
        ice::Span<ice::u32 const> keystringoffsets,
        ice::config::detail::ConfigBuilderContainer const& config,
        ice::config::detail::ConfigKey* out_keylist,
        ice::config::detail::ConfigValue* out_vallist,
        ice::CBDataBuffer& out_data
    ) noexcept -> ice::u32
    {
        // First add all keys-values to the list
        ice::u32 keyoffset = 0;
        ice::config::detail::ConfigBuilderEntry const* it_entry = config._entries.begin();
        ice::config::detail::ConfigBuilderEntry const* const it_end = config._entries.end();
        do
        {
            // Copy the whole key information and just update the str offset.
            out_keylist[keyoffset] = {
                .next = (it_entry + 1) != it_end,
                .type = it_entry->type,
                .vtype = it_entry->vtype,
                .offset = it_entry->offset,
                .size = it_entry->size
            };

            if (config.vtype != CONFIG_VALTYPE_TABLE)
            {
                ice::String const it_keystr = cb_getkey(config._keystrings, *it_entry);
                ice::u32 const it_keystr_idx = ice::cb_find_keystr_idx(keystrings, it_keystr);
                ICE_ASSERT_CORE(it_keystr_idx < ice::hashmap::count(keystrings));
                ice::u32 const new_keystr_offset = keystringoffsets[it_keystr_idx];
                out_keylist[keyoffset].offset = new_keystr_offset;
            }

            it_entry += 1;
            keyoffset += 1;
        }
        while(it_entry != it_end); // We need to check the previous value

        // Then add all children key-values
        ice::u32 out_keyidx = 0;
        it_entry = config._entries.begin();

        if (config.vtype == CONFIG_VALTYPE_TABLE)
        {
            ice::u32 const table_size = config._entries.size().u32();

            // Save table size in first key
            out_keylist[0].offset = table_size >> 8;
            out_keylist[0].size = (table_size & 0xff);
        }

        do
        {
            // Just assign the value to non-container keys
            if (it_entry->vtype < CONFIG_VALTYPE_LARGE)
            {
                // Assign the value always, even if outdated. We will update it later (TODO:)
                out_vallist[out_keyidx].internal = it_entry->data.val_u32;
            }
            else if (it_entry->vtype < CONFIG_VALTYPE_OTHER)
            {
                ICE_ASSERT_CORE(out_data.offset % 8 == 0);

                // Set the value to the current 'offset' value.
                out_vallist[out_keyidx].internal = out_data.offset;

                ice::memcpy(ice::ptr_add(out_data.memory, {out_data.offset}), ice::data_view(it_entry->data.val_u64));
                out_data.offset += 8;
            }
            else if (it_entry->vtype == CONFIG_VALTYPE_STRING)
            {
                ice::HeapVarString<> const& varstr = *config._entries[out_keyidx].data.val_varstr;
                ice::Data const in_data = varstr.data_view();

                // Set the '\0' character
                *(reinterpret_cast<char*>(out_data.memory.location) + (out_data.offset_strings - 1)) = '\0';

                // Update strings offset (including '\0')
                out_data.offset_strings -= (ice::u32) (in_data.size.value + 1);

                // Set the value to the current 'offset_strings' value.
                out_vallist[out_keyidx].internal = out_data.offset_strings;
                ice::memcpy(ice::ptr_add(out_data.memory, {out_data.offset_strings}), in_data);
            }
            else if (it_entry->vtype < CONFIG_VALTYPE_CONTAINER)
            {
                ICE_ASSERT_CORE(false); // TODO
            }
            else // Only for containers
            {
                ConfigBuilderContainer& sub = *config._entries[out_keyidx].data.val_container;

                // Set the value to the current 'keyoffset' value. This serves as the relative offset starting from the passed key.
                out_vallist[out_keyidx].internal = sub._entries.is_empty() ? ice::u32_max : keyoffset - out_keyidx;

                if (out_vallist[out_keyidx].internal != ice::u32_max)
                {
                    // We go through all keys one by one, because we can have holes in the builder.
                    //  We increase the 'keyoffset' by the number of keys updated by the child
                    keyoffset += cb_finalize_store_keysvalues(
                        keystrings,
                        keystringoffsets,
                        sub,
                        out_keylist + keyoffset,
                        out_vallist + keyoffset,
                        out_data
                    );
                }
            }

            it_entry += 1;
            // it_val += 1;
            out_keyidx += 1;
        }
        while(it_entry != it_end);

        return keyoffset;
    }

    void foreach_object_entry(
        ice::ConfigBuilderValue out_builder,
        ice::Config const& config,
        ice::config::detail::ConfigKey const* key,
        ice::config::detail::ConfigValue const* val
    ) noexcept;

    void foreach_table_entry(
        ice::ConfigBuilderValue out_builder,
        ice::Config const& config,
        ice::config::detail::ConfigKey const* key,
        ice::config::detail::ConfigValue const* val
    ) noexcept;

    void set_object_entry(
        ice::ConfigBuilderValue& out_builder,
        ice::Config const& config,
        ice::config::detail::ConfigKey const* key,
        ice::config::detail::ConfigValue const* val
    ) noexcept
    {
    }

    void foreach_table_entry(
        ice::ConfigBuilderValue out_builder,
        ice::Config const& config,
        ice::config::detail::ConfigKey const* key,
        ice::config::detail::ConfigValue const* val
    ) noexcept
    {
        ice::config::detail::array_first(key, val);

        ice::u32 idx = 0;
        do
        {
            switch (ice::config::detail::gettype(key))
            {
                using enum ice::ConfigValueType;
            case Object: foreach_object_entry(out_builder[idx], config, key, val);
            case Table: foreach_table_entry(out_builder[idx], config, key, val);
            case Blob:
            case Invalid: continue;
            default: continue; // TODO: Set value
            }

        } while (ice::config::detail::array_next(key, val) != E_Fail);
    }

    void foreach_object_entry(
        ice::ConfigBuilderValue out_builder,
        ice::Config const& config,
        ice::config::detail::ConfigKey const* key,
        ice::config::detail::ConfigValue const* val
    ) noexcept
    {
        ice::config::detail::entry_first(key, val);

        do
        {
            ice::String const keyval = ice::config::detail::entry_key(config, *key);

            switch (ice::config::detail::gettype(key))
            {
                using enum ice::ConfigValueType;
            case Object: foreach_object_entry(out_builder[keyval], config,  key, val);
            case Table: foreach_table_entry(out_builder[keyval], config, key, val);
            case Blob:
            case Invalid: continue;
            default: continue; // TODO: Set value
            }

        } while (ice::config::detail::entry_next(key, val) != E_Fail);
    }

    auto ConfigBuilder::merge(ice::String json) noexcept -> ice::ErrorCode
    {
        return ice::config::from_json(*this, json);
    }

    auto ConfigBuilder::merge(ice::Config const& config) noexcept -> ice::ErrorCode
    {
        ice::config::detail::ConfigKey const* key = config._keys;
        ice::config::detail::ConfigValue const* val = config._values;

        ICE_ASSERT_CORE(false); // TODO: Unfinished!
        foreach_object_entry(*this, config, key, val);
        return S_Ok;
    }


    auto ConfigBuilder::finalize(ice::Allocator& alloc) noexcept -> ice::Memory
    {
        using ice::config::detail::ConfigKey;
        using ice::config::detail::ConfigValue;

        ice::config::detail::ConfigBuilderContainer& container = *_internal->data.val_container;
        if (container._entries.is_empty())
        {
            return {};
        }

        ice::Array<ice::u32> keyoffsets{ alloc };
        ice::HashMap<ice::CBKeyString> keystrings{ alloc };

        ice::u32 final_count = 0;
        ice::usize const final_size = cb_calculate_final_size(keystrings, container, final_count);

        static_assert(ice::size_of<ConfigKey> + ice::size_of<ConfigValue> == 8_B);
        ice::usize const keyvalue_size = ice::size_of<ConfigKey> + ice::size_of<ConfigValue>;

        // Not sure when that could happen
        if (final_count == 0)
        {
            return {};
        }

        // Alloc the final buffer
        ice::Memory final_buffer = alloc.allocate(final_size);
        ice::Memory final_keystrings_mem = ice::ptr_add(final_buffer, keyvalue_size * (final_count + 2)); // +2 is root and sentiel
        char const* final_keystrings = reinterpret_cast<char const*>(final_keystrings_mem.location);

        // Reserve space to hold all keystring entries and build the string buffer.
        keyoffsets.resize(ice::hashmap::count(keystrings));

        ice::ncount keystr_offset = 0;
        for (CBKeyString const& keystr : ice::hashmap::values(keystrings))
        {
            // Copy and advance the pointer
            ice::memcpy(final_keystrings_mem, keystr.value.data_view());
            final_keystrings_mem = ice::ptr_add(final_keystrings_mem, keystr.value.size());

            // Store the offset
            keyoffsets[keystr.index] = keystr_offset.u32();
            keystr_offset += keystr.value.size();
        }

        // Build the new key and value arrays + copy large data.
        ConfigKey* new_keys = reinterpret_cast<ConfigKey*>(final_buffer.location);

        // First key is the root key (It has no value attached!)
        new_keys[0] = ConfigKey{ .next = 1, .type = CONFIG_KEYTYPE_NONE, .vtype = CONFIG_VALTYPE_ROOT, .offset = final_count >> 8, .size = final_count & 0xff };

        ConfigValue* new_values = reinterpret_cast<ConfigValue*>(new_keys + final_count + 2);

        ice::usize const keystrings_end = ice::ptr_distance(final_buffer.location, final_keystrings_mem.location);
        ice::usize const data_start = ice::align_to(keystrings_end, ice::ualign::b_8).value;
        new_values[0].internal = (ice::u32) data_start.value;

        ICE_ASSERT_CORE((void*)(new_values + final_count + 2) == (void*)final_keystrings);

        ice::CBDataBuffer new_data{
            .memory = ice::ptr_adv(final_buffer, data_start, ice::ualign::b_8),
            .offset = 0,
            .offset_strings = ice::u32(final_size.value - data_start.value)
        };

        ice::u32 const updated_count = cb_finalize_store_keysvalues(
            keystrings,
            keyoffsets,
            container,
            new_keys + 1,
            new_values + 1,
            new_data
        );

        new_keys[final_count + 1] = {};
        new_values[final_count + 1] = {};
        ICE_ASSERT_CORE(updated_count == final_count);

        return final_buffer;
    }

} // namespace ice
