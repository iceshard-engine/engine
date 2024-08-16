#include "config_new.hxx"
#include <ice/config_new_builder.hxx>
#include <ice/config_new_builder_v2.hxx>
#include <ice/container/hashmap.hxx>

namespace ice
{

#if 0
    struct KeyString
    {
        ice::String value;
        ice::u32 index;
    };

    struct DataBuffer
    {
        ice::Memory memory;
        ice::u32 offset;
        ice::u32 offset_strings;
    };

    auto find_keystr_idx(ice::HashMap<KeyString> const& keystrings, ice::String keystr) noexcept -> ice::u32
    {
        ice::u64 const keystr_hash = ice::hash(keystr);
        auto it = ice::multi_hashmap::find_first(keystrings, keystr_hash);
        while(it != nullptr && it.value().value != keystr)
        {
            it = ice::multi_hashmap::find_next(keystrings, it);
        }

        return it == nullptr ? ice::hashmap::count(keystrings) : it.value().index;
    }

    auto cb_v2_find_keystr_idx(ice::HashMap<KeyString> const& keystrings, ice::String keystr) noexcept -> ice::u32
    {
        ice::u64 const keystr_hash = ice::hash(keystr);
        auto it = ice::multi_hashmap::find_first(keystrings, keystr_hash);
        while(it != nullptr && it.value().value != keystr)
        {
            it = ice::multi_hashmap::find_next(keystrings, it);
        }

        return it == nullptr ? ice::hashmap::count(keystrings) : it.value().index;
    }

    auto calculate_key_size(
        ice::HashMap<KeyString>& keystrings,
        ice::ConfigBuilder const& config,
        ice::ConfigBuilder::Entry const* entry
    ) noexcept -> ice::usize
    {
        ice::usize result = 8_B; // Key + Value
        if (entry->type == CONFIG_KEYTYPE_STRING)
        {
            // Find the keystring in the hashmap
            ice::String const keystr = ConfigBuilder::Entry::key(config._keystrings, *entry);
            ice::u32 const keystr_idx = cb_v2_find_keystr_idx(keystrings, keystr);

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

    auto cb_v2_calculate_final_size(
        ice::HashMap<KeyString>& keystrings,
        ice::ConfigBuilder const& config,
        ice::usize& out_data_size,
        ice::u32& out_count
    ) noexcept -> ice::usize
    {
        ice::usize result = 0_B;
        ConfigBuilder::Entry const* entry = ice::array::begin(config._entries);
        ConfigBuilder::Entry const* const entry_end = ice::array::end(config._entries);

        if (entry == entry_end)
        {
            return result;
        }

        do
        {
            result += calculate_key_size(keystrings, config, entry); // key + val

            if (entry->vtype >= CONFIG_VALTYPE_CONTAINER)
            {
                // We go through all keys one by one, because we can have holes in the builder.
                result += cb_v2_calculate_final_size(
                    keystrings,
                    *entry->data.sub,
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
                ice::u32 const strsize = calc_varstring_size_total(entry->size);
                out_data_size += { strsize + 1 }; // +1 for '\0'
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

    auto cb_v2_calculate_final_size(
        ice::HashMap<KeyString>& keystrings,
        ice::ConfigBuilder const& config,
        ice::u32& out_count
    ) noexcept -> ice::usize
    {
        ice::usize data_size = 0_B;
        // We also need to add 16 bytes for both sentiel and root key-value pairs (2 * 2 * 4)
        ice::usize result = 16_B + cb_v2_calculate_final_size(keystrings, config, data_size, out_count);

        if (data_size > 0_B)
        {
            result = ice::align_to(result, ice::ualign::b_8).value;
            result += data_size;
        }

        return result;
    }

    auto cb_v2_store_keysvalues(
        ice::HashMap<KeyString>& keystrings,
        ice::Span<ice::u32 const> keystringoffsets,
        ice::ConfigBuilder const& config,
        ice::Config_v2::Key* out_keylist,
        ice::Config_v2::Value* out_vallist,
        ice::DataBuffer& out_data
    ) noexcept -> ice::u32
    {
        // First add all keys-values to the list
        ice::u32 keyoffset = 0;
        ice::ConfigBuilder::Entry const* it_entry = ice::array::begin(config._entries);
        ice::ConfigBuilder::Entry const* const it_end = ice::array::end(config._entries);
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

            if (config._type != ConfigType::Table)
            {
                ice::String const it_keystr = ConfigBuilder::Entry::key(config._keystrings, *it_entry);
                ice::u32 const it_keystr_idx = ice::find_keystr_idx(keystrings, it_keystr);
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
        it_entry = ice::array::begin(config._entries);

        if (config._type == ConfigType::Table)
        {
            ice::u32 const table_size = ice::count(config._entries);

            // Safe table size in first key
            out_keylist[0].offset = table_size >> 8;
            out_keylist[0].size = (table_size & 0xff);
        }

        // ice::Config_v2::Value const* it_val = config._values + (config._keys - it_key);
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
                ice::u32 bytes = 0;
                ice::u32 const in_str_size = ice::read_varstring_size(it_entry->data.val_varstr.ptr, bytes);

                ice::Data const in_data{
                    .location = it_entry->data.val_varstr.ptr,
                    .size = { bytes + in_str_size + 1 }, // Include the '\0' when copying
                    .alignment = ice::ualign::b_1
                };

                // Update strings offset (including '\0')
                out_data.offset_strings -= (ice::u32) in_data.size.value;

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
                ConfigBuilder& sub = *config._entries[out_keyidx].data.sub;

                // Set the value to the current 'keyoffset' value. This serves as the relative offset starting from the passed key.
                out_vallist[out_keyidx].internal = ice::array::empty(sub._entries) ? ice::u32_max : keyoffset - out_keyidx;

                if (out_vallist[out_keyidx].internal != ice::u32_max)
                {
                    // We go through all keys one by one, because we can have holes in the builder.
                    //  We increase the 'keyoffset' by the number of keys updated by the child
                    keyoffset += cb_v2_store_keysvalues(
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

    auto get_or_set(ice::Allocator& alloc, ice::ConfigBuilder::Entry& entry, ice::ConfigType type) noexcept -> ice::ConfigBuilder&
    {
        ice::u32 const vtype = type == ConfigType::Object ? CONFIG_VALTYPE_OBJECT : CONFIG_VALTYPE_TABLE;
        if (entry.vtype != vtype)
        {
            if (entry.vtype >= CONFIG_VALTYPE_CONTAINER)
            {
                alloc.destroy(entry.data.sub);
            }

            entry.vtype = vtype;
            entry.data.sub = alloc.create<ConfigBuilder>(alloc, type);
        }
        return *entry.data.sub;
    }

    void ConfigBuilder::set_u32(ice::String key, ice::u32 val) noexcept
    {
        Entry& entry = get_entry(key);
        entry.vtype = CONFIG_VALTYPE_32B_BIT;
        entry.data.val_u32 = val;
    }

    void ConfigBuilder::set_u64(ice::String key, ice::u64 val) noexcept
    {
        Entry& entry = get_entry(key);
        entry.vtype = CONFIG_VALTYPE_64B_BIT;
        entry.data.val_u64 = val;
    }

    void ConfigBuilder::add_f32(ice::f32 val) noexcept
    {
        Entry& entry = get_entry();
        entry.vtype = CONFIG_VALTYPE_32B_BIT | CONFIG_VALTYPE_FP_BIT;
        entry.data.val_f32 = val;
    }

    void ConfigBuilder::add_u8(ice::u8 val) noexcept
    {
        Entry& entry = get_entry();
        entry.vtype = CONFIG_VALTYPE_8B_BIT;
        entry.data.val_u32 = val;
    }

    auto ConfigBuilder::finalize(ice::Allocator &alloc) noexcept -> ice::Memory
    {
        if (ice::array::empty(_entries))
        {
            return {};
        }

        ice::Array<ice::u32> keyoffsets{ alloc };
        ice::HashMap<ice::KeyString> keystrings{ alloc };

        ice::u32 final_count = 0;
        ice::usize const final_size = cb_v2_calculate_final_size(keystrings, *this, final_count);
        ice::usize const keyvalue_size = ice::size_of<Config_v2::Key> + ice::size_of<Config_v2::Value>;

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
        ice::array::resize(keyoffsets, ice::hashmap::count(keystrings));

        ice::u32 keystr_offset = 0;
        for (KeyString const& keystr : ice::hashmap::values(keystrings))
        {
            // Copy and advance the pointer
            ice::memcpy(final_keystrings_mem, ice::string::data_view(keystr.value));
            final_keystrings_mem = ice::ptr_add(final_keystrings_mem, ice::string::data_view(keystr.value).size);

            // Store the offset
            keyoffsets[keystr.index] = keystr_offset;
            keystr_offset += ice::string::size(keystr.value);
        }

        // Build the new key and value arrays + copy large data.
        ice::Config_v2::Key* new_keys = reinterpret_cast<ice::Config_v2::Key*>(final_buffer.location);

        // First key is the root key (It has no value attached!)
        new_keys[0] = ice::Config_v2::Key{ .next = 1, .type = 0, .vtype = CONFIG_VALTYPE_ROOT, .offset = final_count >> 8, .size = final_count & 0xf };

        ice::Config_v2::Value* new_values = reinterpret_cast<ice::Config_v2::Value*>(new_keys + final_count + 2);

        ice::usize const keystrings_end = ice::ptr_distance(final_buffer.location, final_keystrings_mem.location);
        ice::usize const data_start = ice::align_to(keystrings_end, ice::ualign::b_8).value;
        new_values[0].internal = (ice::u32) data_start.value;

        ICE_ASSERT_CORE((void*)(new_values + final_count + 2) == (void*)final_keystrings);

        ice::DataBuffer new_data{
            .memory = ice::ptr_adv(final_keystrings_mem, 0_B, ice::ualign::b_8),
            .offset = 0,
            .offset_strings = ice::u32(final_size.value - new_values[0].internal),
        };

        ice::u32 const updated_count = cb_v2_store_keysvalues(
            keystrings,
            keyoffsets,
            *this,
            new_keys + 1,
            new_values + 1,
            new_data
        );

        new_keys[final_count + 1] = {};
        new_values[final_count + 1] = {};
        ICE_ASSERT_CORE(updated_count == final_count);

        return final_buffer;
    }

    auto ConfigBuilder::operator[](ice::String key) noexcept -> Value
    {
        return { ice::addressof(_allocator), ice::addressof(get_entry(key, false)) };
    }

    auto ConfigBuilder::Value::operator[](ice::String key) noexcept -> ice::ConfigBuilder::Value
    {
        return get_or_set(*alloc, *entry, ConfigType::Object)[key];
    }

    template<>
    void ConfigBuilder::Value::set<ice::i32>(ice::i32 value) noexcept
    {
        ICE_ASSERT_CORE(entry != nullptr);
        entry->vtype = CONFIG_VALTYPE_32B_BIT | CONFIG_VALTYPE_SIGN_BIT;
        entry->data.val_u64 = value;
    }

    template<>
    void ConfigBuilder::Value::set<ice::u32>(ice::u32 value) noexcept
    {
        ICE_ASSERT_CORE(entry != nullptr);
        entry->vtype = CONFIG_VALTYPE_32B_BIT;
        entry->data.val_u64 = value;
    }
#endif

} // namespace ice
