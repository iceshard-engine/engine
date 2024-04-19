#include "config_new.hxx"
#include <ice/config_new_builder.hxx>
#include <ice/container/hashmap.hxx>

namespace ice
{

    namespace config_builder
    {

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

        auto calculate_key_size(
            ice::HashMap<KeyString>& keystrings,
            ice::ConfigBuilder_v2 const& config,
            ice::Config_v2::Key const* key
        ) noexcept -> ice::usize
        {
            ice::usize result = 8_B;
            if (key->type == CONFIG_KEYTYPE_STRING)
            {
                // Find the keystring in the hashmap
                ice::String const keystr{ config._strings + key->offset, key->size };
                ice::u32 const keystr_idx = find_keystr_idx(keystrings, keystr);

                // If it's not store a new entry and give it a new index.
                //   This allows us to reuse duplicate key names
                if (keystr_idx == ice::hashmap::count(keystrings))
                {
                    ice::multi_hashmap::insert(keystrings, ice::hash(keystr), { keystr, keystr_idx });
                    result += { key->size }; // Increase the size required
                }
            }
            return result;
        }

        auto calculate_final_size(
            ice::HashMap<KeyString>& keystrings,
            ice::ConfigBuilder_v2 const& config,
            ice::Config_v2::Key const* key,
            ice::usize& out_data_size,
            ice::u32& out_count
        ) noexcept -> ice::usize
        {
            ice::usize result = 0_B;

            do
            {
                result += calculate_key_size(keystrings, config, key); // key + val

                ice::usize_raw const idx = key - config._keys;
                if (key->vtype >= CONFIG_VALTYPE_CONTAINER)
                {
                    // We go through all keys one by one, because we can have holes in the builder.
                    result += calculate_final_size(
                        keystrings,
                        config,
                        config._keys + idx + config._values[idx].internal,
                        out_data_size,
                        out_count
                    );
                }
                else if (key->vtype == CONFIG_VALTYPE_STRING)
                {
                    ice::u32 bytes = 0;
                    ice::u32 const strsize = read_varstring_size(ice::ptr_add(config._data, { config._values[idx].internal }), bytes);
                    out_data_size += { bytes + strsize + 1 };
                }
                else if (key->vtype >= CONFIG_VALTYPE_LARGE)
                {
                    out_data_size += 8_B;
                }

                key += 1;
                out_count += 1;
            }
            while(key[-1].next);

            // Config_v2::Key const* final_key = config._keys + config._count;
            return result;
        }

        auto calculate_final_size(
            ice::HashMap<KeyString>& keystrings,
            ice::ConfigBuilder_v2 const& config,
            ice::Config_v2::Key const* key,
            ice::u32& out_count
        ) noexcept -> ice::usize
        {
            ice::usize data_size = 0_B;
            // We also need to add 16 bytes for both sentiel and root key-value pairs (2 * 2 * 4)
            ice::usize result = 16_B + calculate_final_size(keystrings, config, key, data_size, out_count);

            if (data_size > 0_B)
            {
                result = ice::align_to(result, ice::ualign::b_8).value;
                result += data_size;
            }

            return result;
        }

        auto store_keysvalues(
            ice::HashMap<KeyString>& keystrings,
            ice::Span<ice::u32 const> keystringoffsets,
            ice::ConfigBuilder_v2 const& config,
            ice::Config_v2::Key const* in_key,
            ice::Config_v2::Key* out_keylist,
            ice::Config_v2::Value* out_vallist,
            config_builder::DataBuffer& out_data
        ) noexcept -> ice::u32
        {
            // First add all keys-values to the list
            ice::u32 keyoffset = 0;
            ice::Config_v2::Key const* it_key = in_key;
            do
            {
                ice::String const it_keystr{ config._strings + it_key->offset, it_key->size };
                ice::u32 const it_keystr_idx = config_builder::find_keystr_idx(keystrings, it_keystr);
                ICE_ASSERT_CORE(it_keystr_idx < ice::hashmap::count(keystrings));
                ice::u32 const new_keystr_offset = keystringoffsets[it_keystr_idx];

                // Copy the whole key information and just update the str offset.
                out_keylist[keyoffset] = *it_key;
                out_keylist[keyoffset].offset = new_keystr_offset;

                it_key += 1;
                keyoffset += 1;
            }
            while(it_key[-1].next); // We need to check the previous value

            // Then add all children key-values
            ice::u32 out_keyidx = 0;
            it_key = in_key;

            // ice::Config_v2::Value const* it_val = config._values + (config._keys - it_key);
            do
            {
                ice::usize_raw const idx = it_key - config._keys;

                // Just assign the value to non-container keys
                if (it_key->vtype < CONFIG_VALTYPE_LARGE)
                {
                    // Assign the value always, even if outdated. We will update it later (TODO:)
                    out_vallist[out_keyidx] = config._values[idx];
                }
                else if (it_key->vtype < CONFIG_VALTYPE_OTHER)
                {
                    ICE_ASSERT_CORE(out_data.offset % 8 == 0);

                    ice::u32 const in_value_offset = config._values[idx].internal;
                    ice::Data const in_data{
                        .location = ice::ptr_add(config._data, {in_value_offset}),
                        .size = 8_B,
                        .alignment = ice::ualign::b_8
                    };

                    // Set the value to the current 'offset' value.
                    out_vallist[out_keyidx].internal = out_data.offset;

                    ice::memcpy(ice::ptr_add(out_data.memory, {out_data.offset}), in_data);
                    out_data.offset += 8;
                }
                else if (it_key->vtype == CONFIG_VALTYPE_STRING)
                {
                    ice::u32 const in_value_offset = config._values[idx].internal;
                    void const* in_data_ptr = ice::ptr_add(config._data, {in_value_offset});

                    ice::u32 bytes = 0;
                    ice::u32 const in_str_size = ice::read_varstring_size(in_data_ptr, bytes);

                    ice::Data const in_data{
                        .location = in_data_ptr,
                        .size = bytes + in_str_size + 1, // Include the '\0' when copying
                        .alignment = ice::ualign::b_1
                    };

                    // Update strings offset (including '\0')
                    out_data.offset_strings -= (ice::u32) in_data.size.value;

                    // Set the value to the current 'offset_strings' value.
                    out_vallist[out_keyidx].internal = out_data.offset_strings;
                    ice::memcpy(ice::ptr_add(out_data.memory, {out_data.offset_strings}), in_data);
                }
                else if (it_key->vtype < CONFIG_VALTYPE_CONTAINER)
                {
                    ICE_ASSERT_CORE(false); // TODO
                }
                else // Only for containers
                {
                    // Set the value to the current 'keyoffset' value. This serves as the relative offset starting from the passed key.
                    out_vallist[out_keyidx].internal = keyoffset - out_keyidx;

                    // We go through all keys one by one, because we can have holes in the builder.
                    //  We increase the 'keyoffset' by the number of keys updated by the child
                    keyoffset += store_keysvalues(
                        keystrings,
                        keystringoffsets,
                        config,
                        config._keys + idx + config._values[idx].internal,
                        out_keylist + keyoffset,
                        out_vallist + keyoffset,
                        out_data
                    );
                }

                it_key += 1;
                // it_val += 1;
                out_keyidx += 1;
            }
            while(it_key[-1].next);

            return keyoffset;
        }

    } // namespace detail

    void configbuilder_grow_keyvals(ice::ConfigBuilder_v2& config) noexcept;

    ice::ConfigBuilder_v2::ConfigBuilder_v2(ice::Allocator& alloc) noexcept
        : _keys{ nullptr }
        , _values{ nullptr }
        , _strings{ nullptr }
        , _data{ nullptr }
        , _count{ 0 }
        , _capacity{ 0 }
        , _keyval_buffer{ ice::addressof(alloc) }
        , _keystr_buffer{ ice::addressof(alloc) }
        , _data_buffer{ ice::addressof(alloc) }
    {
        configbuilder_grow_keyvals(*this);
        _keys[0] = Config_v2::Key{};
    }

    void configbuilder_grow_keyvals(ice::ConfigBuilder_v2& config) noexcept
    {
        ice::buffer::grow(config._keyval_buffer, 128_B);
        ice::usize const cap_new = ice::buffer::capacity(config._keyval_buffer);
        ice::usize_raw const capacity_new = cap_new.value / (4 + 4); // Both keys and values are 8 bytes large.

        config._capacity = ice::ucount(capacity_new - 1); // The final key/value is always a sentiel value so we don't count it into capacity

        // Reassing the keys pointer
        config._keys = reinterpret_cast<Config_v2::Key*>(config._keyval_buffer.memory.location);

        // Reassing the values pointer and move values to their new location.
        config._values = reinterpret_cast<Config_v2::Value*>(config._keys + capacity_new);

        Config_v2::Value* old_values_end = reinterpret_cast<Config_v2::Value*>(config._keys + config._count * 2);
        Config_v2::Value* new_values_end = config._values + config._count;
        ICE_ASSERT_CORE(new_values_end > old_values_end);

        ice::ucount values_to_move = config._count;
        while(values_to_move > 0)
        {
            *new_values_end = *old_values_end;
            new_values_end -= 1;
            old_values_end -= 1;
            values_to_move -= 1;
        }

        // Set the sentiel values
        config._keys[config._count] = {};
        config._values[config._count] = {};
    }

    void confibbuilder_move_keyvals(ice::ConfigBuilder_v2& config, ice::u32 start_index, ice::u32 offset = 1) noexcept
    {
        if (config._count == config._capacity)
        {
            configbuilder_grow_keyvals(config);
        }

        ice::u32 count = config._count - start_index;
        ICE_ASSERT_CORE(config._count >= count);

        Config_v2::Key* keys_end = config._keys + config._count;
        Config_v2::Value* values_end = config._values + config._count;

        while(count > 0)
        {
            *keys_end = *(keys_end - offset);
            *values_end = *(values_end - offset);
            keys_end -= 1;
            values_end -= 1;
            count -= 1;
        }
    }

    void configbuilder_clear_subkeyvals(ice::ConfigBuilder_v2& config, Config_v2::Key* key) noexcept
    {
        // Only for tables and objects
        if (key->vtype < CONFIG_VALTYPE_TABLE)
        {
            return;
        }

        ice::usize_raw const idx = key - config._keys;
        ice::usize_raw const offset = idx + config._values[idx].internal;

        // Walks over all keys in the sub-object
        Config_v2::Key* subkey = config._keys + offset;
        while(subkey->next)
        {
            // Clear any sub-keys if necessary
            configbuilder_clear_subkeyvals(config, subkey);
            *subkey = Config_v2::Key{};
            subkey += 1;
        }

        // Clear the last one
        configbuilder_clear_subkeyvals(config, subkey);
        *subkey = Config_v2::Key{};
    }

    void configbuilder_grow_keystr(ice::ConfigBuilder_v2& config, ice::usize new_required) noexcept
    {
        ice::buffer::grow(config._keystr_buffer, ice::buffer::capacity(config._keystr_buffer) + new_required);

        // Update the config pointer
        config._strings = reinterpret_cast<char const*>(config._keystr_buffer.memory.location);
    }

    auto configbuilder_store_keystr(ice::ConfigBuilder_v2& config, ice::String keystr) noexcept -> ice::u32
    {
        ice::usize const required_space = { ice::string::size(keystr) };
        ICE_ASSERT_CORE(required_space <= 255_B);
        if (ice::buffer::space(config._keystr_buffer) < required_space)
        {
            configbuilder_grow_keystr(config, required_space);
        }

        // Store the string
        ice::Memory mem = ice::buffer::append_reserve(config._keystr_buffer, ice::meminfo{ .size = required_space, .alignment = ice::ualign::b_1 });
        ice::memcpy(mem, ice::string::data_view(keystr));

        return (ice::u32) ice::ptr_distance(config._keystr_buffer.memory.location, mem.location).value;
    }

    bool configbuilder_getsimple_key(ice::ConfigBuilder_v2& config, ice::u32 offset, ice::String key, Config_v2::Key*& out_key) noexcept
    {
        // Try to find the subkey
        out_key = config._keys + offset;
        if (out_key->type != CONFIG_KEYTYPE_STRING)
        {
            return false;
        }

        bool found = key == Config_v2::Key::key_str(*out_key, config._strings);

        while(out_key->next && found == false)
        {
            out_key += 1;
            found = key == Config_v2::Key::key_str(*out_key, config._strings);
        }

        return found;
    }

    auto configbuilder_get_key(ice::ConfigBuilder_v2& config, ice::String key) noexcept -> Config_v2::Key*
    {
        // If this key has multiple parts recursively enter config search
        ice::u32 key_group_offset = 0;
        ice::ucount key_split_location = ice::string::find_first_of(key, '.');

        bool found = true;
        while(key_split_location != ice::String_NPos && found)
        {
            String subkey = ice::string::substr(key, 0, key_split_location);

            // Try to find the subkey
            Config_v2::Key* key_info = nullptr;
            found = configbuilder_getsimple_key(config, key_group_offset, subkey, key_info);

            // If found, move up the keychain
            if (found)
            {
                ice::u32 const key_idx = ice::u32(key_info - config._keys);

                ICE_ASSERT_CORE(key_info->vtype == CONFIG_VALTYPE_OBJECT);
                key_group_offset = key_idx + (config._values + key_idx)->internal; // Offset into the objects keys

                key = ice::string::substr(key, key_split_location + 1);
                key_split_location = ice::string::find_first_of(key, '.');
            }
        }

        if (found == false)
        {
            return nullptr;
        }

        // Try to find the final key
        Config_v2::Key* key_info = nullptr;
        found = configbuilder_getsimple_key(config, key_group_offset, key, key_info);

        return found ? key_info : nullptr;
    }

    auto configbuilder_setorinsert_key(ice::ConfigBuilder_v2& config, Config_v2::Key* key_info, ice::String key) noexcept -> Config_v2::Key*
    {
        ice::u32 const key_idx = ice::u32(key_info - config._keys);

        ice::u32 moved = 0;
        if (config._count == key_idx)
        {
            moved = 1;
        }
        else if (key_info[1].vtype != CONFIG_VALTYPE_NONE)
        {
            // We need to make space after the last entry
            confibbuilder_move_keyvals(config, key_idx + 1);
            moved = 1;
        }

        ice::i32 count_parents = key_idx;
        while(count_parents >= 0)
        {
            if (config._keys[count_parents].vtype >= CONFIG_VALTYPE_TABLE)
            {
                // need to move all offsets by one if we inserted in the middle
                if (count_parents + config._values[count_parents].internal > key_idx)
                {
                    config._values[count_parents].internal += moved;
                }
            }
            count_parents -= 1;
        }

        // If we are added at the end
        if (key_info->vtype != CONFIG_VALTYPE_NONE && key_info->next == 0)
        {
            // Update the key infos now
            key_info->next = 1;
            key_info += 1; // Now fill the next keyinfo

            moved += (config._count == (key_info - config._keys));
        }

        key_info->next = 0;
        key_info->type = CONFIG_KEYTYPE_STRING;
        key_info->vtype = CONFIG_VALTYPE_OBJECT;
        key_info->size = ice::string::size(key);
        key_info->offset = configbuilder_store_keystr(config, key);

        // Increment count for keys / vals
        config._count += moved;
        config._keys[config._count] = Config_v2::Key{};
        return key_info;
    }

    auto configbuilder_getorcreate_key(ice::ConfigBuilder_v2& config, ice::String key) noexcept -> Config_v2::Key*
    {
        // If this key has multiple parts recursively enter config search
        // TODO: Turn into a loop?
        ice::u32 key_group_offset = 0;
        ice::ucount key_split_location = ice::string::find_first_of(key, '.');
        while(key_split_location != ice::String_NPos)
        {
            String subkey = ice::string::substr(key, 0, key_split_location);

            // Try to find the subkey
            Config_v2::Key* key_info = nullptr;
            bool const found = configbuilder_getsimple_key(config, key_group_offset, subkey, key_info);

            // If found, move up the keychain
            if (found)
            {
                ice::u32 const key_idx = ice::u32(key_info - config._keys);

                ICE_ASSERT_CORE(key_info->vtype == CONFIG_VALTYPE_OBJECT);
                key_group_offset = key_idx + (config._values + key_idx)->internal; // Offset into the objects keys
            }
            // Else create the new sub-key
            else
            {
                key_info = configbuilder_setorinsert_key(config, key_info, subkey);

                Config_v2::Value* parent_value = config._values + (key_info - config._keys);
                parent_value->internal = config._count - ice::u32(key_info - config._keys);

                key_group_offset = config._count;
            }

            key = ice::string::substr(key, key_split_location + 1);
            key_split_location = ice::string::find_first_of(key, '.');
        }

        // Try to find the final key
        Config_v2::Key* key_info = nullptr;
        bool const found = configbuilder_getsimple_key(config, key_group_offset, key, key_info);

        if (found == false)
        {
            key_info = configbuilder_setorinsert_key(config, key_info, key);
        }

        return key_info;
    }

    template<typename T>
    auto configbuilder_store_value(ice::ConfigBuilder_v2& config, T value) noexcept -> ice::u32
    {
        if (ice::buffer::has_space(config._data_buffer, ice::meminfo_of<T>) == false)
        {
            ice::buffer::grow(config._data_buffer, 128_B + ice::buffer::required_capacity(config._data_buffer, ice::meminfo_of<T>));

            // Update the config pointer
            config._data = reinterpret_cast<char const*>(config._data_buffer.memory.location);
        }

        // Copy the value into the memory
        ice::Memory mem = ice::buffer::append_reserve(config._data_buffer, ice::meminfo_of<T>);
        ice::memcpy(mem, ice::data_view(value));
        return (ice::u32) ice::ptr_distance(config._data, mem.location).value;
    }

    template<>
    auto configbuilder_store_value<ice::String>(ice::ConfigBuilder_v2& config, ice::String value) noexcept -> ice::u32
    {
        char varstr_bytes[5]{};
        ice::u32 const varstr_size = ice::write_varstring_size(varstr_bytes, ice::string::size(value));

        ice::meminfo const req_size{ { ice::string::size(value) + varstr_size + 1 }, ice::ualign::b_1 };
        if (ice::buffer::has_space(config._data_buffer, req_size) == false)
        {
            ice::buffer::grow(config._data_buffer, 128_B + ice::buffer::required_capacity(config._data_buffer, req_size));

            // Update the config pointer
            config._data = reinterpret_cast<char const*>(config._data_buffer.memory.location);
        }

        // Copy the value into the memory
        ice::Memory mem = ice::buffer::append_reserve(config._data_buffer, req_size);
        ice::memcpy(mem, ice::data_view(varstr_bytes));
        ice::memcpy(ice::ptr_add(mem, {varstr_size}), ice::string::data_view(value));
        ((char*) mem.location)[req_size.size.value - 1] = '\0';
        return (ice::u32) ice::ptr_distance(config._data, mem.location).value;
    }

    bool config_set_i16(ice::ConfigBuilder_v2& config, ice::String key, ice::i16 val) noexcept
    {
        Config_v2::Key* key_info = configbuilder_getorcreate_key(config, key);
        ICE_ASSERT_CORE(key_info != nullptr);

        // Just fill in the values.
        key_info->vtype = CONFIG_VALTYPE_16B_BIT | CONFIG_VALTYPE_SIGN_BIT;
        config._values[key_info - config._keys].internal = std::bit_cast<ice::u16>(val);

        return true;
    }

    bool config_set_u32(ice::ConfigBuilder_v2& config, ice::String key, ice::u32 val) noexcept
    {
        Config_v2::Key* key_info = configbuilder_getorcreate_key(config, key);
        ICE_ASSERT_CORE(key_info != nullptr);

        // Just fill in the values.
        key_info->vtype = CONFIG_VALTYPE_32B_BIT;
        config._values[key_info - config._keys].internal = val;

        return true;
    }

    bool config_set_f64(ice::ConfigBuilder_v2 &config, ice::String key, ice::f64 val) noexcept
    {
        Config_v2::Key* key_info = configbuilder_getorcreate_key(config, key);
        ICE_ASSERT_CORE(key_info != nullptr);

        // Just fill in the values.
        key_info->vtype = CONFIG_VALTYPE_64B_BIT | CONFIG_VALTYPE_FP_BIT | CONFIG_VALTYPE_SIGN_BIT;
        config._values[key_info - config._keys].internal = configbuilder_store_value(config, val);

        return true;
    }

    bool config_set_str(ice::ConfigBuilder_v2& config, ice::String key, ice::String val) noexcept
    {
        Config_v2::Key* key_info = configbuilder_getorcreate_key(config, key);
        ICE_ASSERT_CORE(key_info != nullptr);

        // Just fill in the values.
        key_info->vtype = CONFIG_VALTYPE_STRING;
        config._values[key_info - config._keys].internal = configbuilder_store_value(config, val);
        return true;
    }

    bool config_remove(ice::ConfigBuilder_v2 &config, ice::String key) noexcept
    {
        Config_v2::Key* key_info = configbuilder_get_key(config, key);
        if (key_info != nullptr)
        {
            ice::usize_raw key_idx = key_info - config._keys;
            if (config._keys[key_idx].next == 0 && key_idx > 0)
            {
                // Set the previous keyinfo next value to '0' (can be done always)
                config._keys[key_idx - 1].next = 0;
            }

            // Clear the whole tree if it's an object or array.
            configbuilder_clear_subkeyvals(config, config._keys + key_idx);

            // Move subsequent keys into the removed place.
            while(config._keys[key_idx].next)
            {
                config._keys[key_idx] = config._keys[key_idx + 1];
                config._values[key_idx] = config._values[key_idx + 1];
                key_idx += 1;
            }

            // Clear the last value
            config._keys[key_idx] = Config_v2::Key{};
            config._values[key_idx].internal = 0;
        }
        return key_info != nullptr;
    }

    auto config_finalize(ice::ConfigBuilder_v2& config, ice::Allocator& alloc) noexcept -> ice::Memory
    {
        if (config._count == 0)
        {
            return {};
        }

        ice::Array<ice::u32> keyoffsets{ alloc };
        ice::HashMap<ice::config_builder::KeyString> keystrings{ alloc };

        ice::u32 final_count = 0;
        ice::usize const final_size = config_builder::calculate_final_size(keystrings, config, config._keys, final_count);
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
        for (config_builder::KeyString const& keystr : ice::hashmap::values(keystrings))
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

        config_builder::DataBuffer new_data{
            .memory = ice::ptr_adv(final_keystrings_mem, 0_B, ice::ualign::b_8),
            .offset = 0,
            .offset_strings = ice::u32(final_size.value - new_values[0].internal),
        };

        ice::u32 const updated_count = config_builder::store_keysvalues(
            keystrings,
            keyoffsets,
            config,
            config._keys,
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
