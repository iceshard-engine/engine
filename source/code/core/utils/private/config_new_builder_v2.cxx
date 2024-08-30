#include <ice/config_new_builder_v2.hxx>
#include <ice/container/hashmap.hxx>
#include "config_new.hxx"

namespace ice
{

    struct ConfigBuilderContainer;

    namespace detail
    {

        auto key(ice::String src, ice::ConfigBuilderValue::Entry const& entry) noexcept -> ice::String;

        void assign_value_type(
            ice::Allocator& alloc,
            ice::ConfigBuilderValue::Entry& entry,
            ice::Config_ValType vtype
        ) noexcept;

        void clear_value_type(
            ice::Allocator& alloc,
            ice::ConfigBuilderValue::Entry* entry
        ) noexcept;

        auto get_entry(
            ice::Allocator& alloc,
            ice::ConfigBuilderValue::Entry& entry,
            ice::String key,
            bool clean
        ) noexcept -> ice::ConfigBuilderValue::Entry*;

        auto get_entry(
            ice::Allocator& alloc,
            ice::ConfigBuilderValue::Entry& entry,
            ice::u32 idx,
            bool clean
        ) noexcept -> ice::ConfigBuilderValue::Entry*;

        // auto get_value_entry(
        //     ice::ConfigBuilderValue::Entry* entry,
        //     ice::u32 ref
        // ) noexcept -> ice::ConfigBuilderValue::Entry*;

        auto get_value_entry(
            ice::ConfigBuilderContainer* container,
            ice::u32 ref
        ) noexcept -> ice::ConfigBuilderValue::Entry*;

        auto get_or_set_container(
            ice::Allocator& alloc,
            ice::ConfigBuilderValue::Entry& entry,
            ice::Config_ValType vtype
        ) noexcept -> ice::ConfigBuilderValue;

    } // namespace detail

    struct ConfigBuilderContainer;

    struct ConfigBuilderValue::Entry : ice::Config_v2::Key
    {
        Entry(ice::Config_v2::Key key) noexcept
            : ice::Config_v2::Key{ key }
            , data{ .val_custom = { } }
        { }

        ~Entry() noexcept
        {
        }

        union Data
        {
            ice::u32 val_u32;
            ice::i32 val_i32;
            ice::f32 val_f32;
            ice::u64 val_u64;
            ice::i64 val_i64;
            ice::f64 val_f64;
            ice::VarString val_varstr;
            ice::ConfigBuilderContainer* val_container;
            struct
            {
                void const* location;
                ice::usize size;
            } val_custom;
        } data;
    };

    struct ConfigBuilderContainer : ConfigBuilderValue::Entry
    {
        ConfigBuilderContainer(ice::Allocator& alloc, ice::Config_ValType vtype) noexcept
            : Entry{ { .next = 0, .type = 0, .vtype = vtype, .offset = 0, .size = 0 } }
            , _refcount{ 1 }
            , _entries{ alloc }
            , _keystrings{ alloc }
        {
        }

        auto addref() noexcept -> ConfigBuilderContainer*
        {
            return ++_refcount, this;
        }

        bool release(ice::Allocator& alloc) noexcept
        {
            if (_refcount -= 1; _refcount == 0)
            {
                this->~ConfigBuilderContainer();
                alloc.deallocate(this);
            }
            return _refcount == 0;
        }

        void clear() noexcept
        {
            ice::Allocator& _allocator = *_entries._allocator;
            for (ConfigBuilderValue::Entry& entry : _entries)
            {
                detail::clear_value_type(_allocator, &entry);
            }
            ice::array::clear(_entries);
        }

        ice::u32 _refcount;
        ice::Array<ConfigBuilderValue::Entry> _entries;
        ice::HeapString<> _keystrings;

    private:
        ~ConfigBuilderContainer() noexcept
        {
            clear();
        }
    };

    ConfigBuilderValue::ConfigBuilderValue(ice::Allocator* alloc, Entry* entry, ice::u32 ref) noexcept
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
        ConfigBuilderContainer::Entry* const entry = _idx == ice::u32_max
            ? _internal : (static_cast<ConfigBuilderContainer*>(_internal)->_entries._data + _idx);
        if (entry->vtype == CONFIG_VALTYPE_NONE)
        {
            detail::assign_value_type(*_alloc, *entry, CONFIG_VALTYPE_OBJECT);
        }
        ICE_ASSERT_CORE(entry->vtype >= CONFIG_VALTYPE_OBJECT);

        ConfigBuilderContainer* container = entry->data.val_container;
        ConfigBuilderValue::Entry* value = detail::get_entry(*_alloc, *entry, key, false);

        return ConfigBuilderValue{
            _alloc,
            container,
            (ice::u32)(value - container->_entries._data)
        };
    }

    auto ice::ConfigBuilderValue::operator[](ice::u32 idx) noexcept -> ConfigBuilderValue
    {
        ConfigBuilderContainer::Entry* const entry = _idx == ice::u32_max
            ? _internal : (static_cast<ConfigBuilderContainer*>(_internal)->_entries._data + _idx);
        if (entry->vtype == CONFIG_VALTYPE_NONE)
        {
            detail::assign_value_type(*_alloc, *entry, CONFIG_VALTYPE_TABLE);
        }
        ICE_ASSERT_CORE(entry->vtype == CONFIG_VALTYPE_TABLE);

        ConfigBuilderContainer* container = entry->data.val_container;
        ConfigBuilderValue::Entry* value = detail::get_entry(*_alloc, *entry, idx, false);

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
            detail::clear_value_type(*_alloc, ice::exchange(_internal->data.val_container, nullptr));
        }
    }

    ConfigBuilder_v2::ConfigBuilder_v2(ice::Allocator &alloc) noexcept
        : ConfigBuilderValue{
            ice::addressof(alloc),
            alloc.create<ConfigBuilderContainer>(alloc, CONFIG_VALTYPE_OBJECT),
            ice::u32_max
        }
    {
        _internal->data.val_container = static_cast<ConfigBuilderContainer*>(_internal);
    }

    ice::ConfigBuilder_v2::~ConfigBuilder_v2() noexcept
    {
        detail::clear_value_type(*_alloc, _internal);
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

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
        ice::ConfigBuilderContainer const& config,
        ice::ConfigBuilderValue::Entry const* entry
    ) noexcept -> ice::usize
    {
        ice::usize result = 8_B; // Key + Value
        if (entry->type == CONFIG_KEYTYPE_STRING)
        {
            // Find the keystring in the hashmap
            ice::String const keystr = detail::key(config._keystrings, *entry);
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
        ice::ConfigBuilderContainer const& config,
        ice::usize& out_data_size,
        ice::u32& out_count
    ) noexcept -> ice::usize
    {
        ice::usize result = 0_B;
        ConfigBuilderValue::Entry const* entry = ice::array::begin(config._entries);
        ConfigBuilderValue::Entry const* const entry_end = ice::array::end(config._entries);

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
        ice::ConfigBuilderContainer const& config,
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
        ice::ConfigBuilderContainer const& config,
        ice::Config_v2::Key* out_keylist,
        ice::Config_v2::Value* out_vallist,
        ice::DataBuffer& out_data
    ) noexcept -> ice::u32
    {
        // First add all keys-values to the list
        ice::u32 keyoffset = 0;
        ice::ConfigBuilderValue::Entry const* it_entry = ice::array::begin(config._entries);
        ice::ConfigBuilderValue::Entry const* const it_end = ice::array::end(config._entries);
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
                ice::String const it_keystr = detail::key(config._keystrings, *it_entry);
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

        if (config.vtype == CONFIG_VALTYPE_TABLE)
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
                ConfigBuilderContainer& sub = *config._entries[out_keyidx].data.val_container;

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

    auto ConfigBuilder_v2::finalize(ice::Allocator& alloc) noexcept -> ice::Memory
    {
        ice::ConfigBuilderContainer& container = *_internal->data.val_container;
        if (ice::array::empty(container._entries))
        {
            return {};
        }

        ice::Array<ice::u32> keyoffsets{ alloc };
        ice::HashMap<ice::KeyString> keystrings{ alloc };

        ice::u32 final_count = 0;
        ice::usize const final_size = cb_v2_calculate_final_size(keystrings, container, final_count);
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

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

    namespace detail
    {

        auto key(ice::String src, ice::ConfigBuilderValue::Entry const& entry) noexcept -> ice::String
        {
            return { src._data + entry.offset, entry.size };
        }

        void clear_value_type(
            ice::Allocator& alloc,
            ice::ConfigBuilderValue::Entry* entry
        ) noexcept
        {
            if (entry)
            {
                bool clear_vtype = true;
                if (entry->vtype >= CONFIG_VALTYPE_CONTAINER)
                {
                    clear_vtype = entry->data.val_container->release(alloc);
                }

                if (clear_vtype)
                {
                    entry->vtype = CONFIG_VALTYPE_NONE;
                }
            }
        }

        void assign_value_type(
            ice::Allocator& alloc,
            ice::ConfigBuilderValue::Entry& entry,
            ice::Config_ValType vtype
        ) noexcept
        {
            clear_value_type(alloc, &entry);
            entry.vtype = vtype;
            if (entry.vtype >= CONFIG_VALTYPE_CONTAINER)
            {
                entry.data.val_container = alloc.create<ConfigBuilderContainer>(alloc, vtype);
            }
        }

        // auto get_or_set_container(
        //     ice::Allocator& alloc,
        //     ice::ConfigBuilderValue::Entry& entry,
        //     ice::Config_ValType vtype
        // ) noexcept -> ice::ConfigBuilderValue
        // {
        //     if (entry.vtype != vtype)
        //     {
        //         detail::assign_value_type(alloc, entry, vtype);
        //     }
        //     return { ice::addressof(alloc), entry.data.val_container, 0 };
        // }

        auto get_entry(
            ice::Allocator& alloc,
            ice::ConfigBuilderValue::Entry& entry,
            ice::String key,
            bool clean
        ) noexcept -> ice::ConfigBuilderValue::Entry*
        {
            ICE_ASSERT_CORE(entry.vtype == CONFIG_VALTYPE_OBJECT);
            ice::ConfigBuilderContainer& container = *entry.data.val_container;

            ice::u32 idx = 0;
            for (;idx < ice::count(container._entries); ++idx)
            {
                ice::String const entry_key = detail::key(container._keystrings, container._entries[idx]);
                if (key == entry_key)
                {
                    break;
                }
            }

            ice::ConfigBuilderValue::Entry* result;
            if (idx < ice::count(container._entries))
            {
                result = ice::array::begin(container._entries) + idx;
                if (clean)
                {
                    clear_value_type(alloc, result);
                }
            }
            else
            {
                idx = ice::count(container._entries);
                ice::u32 const offset = ice::string::size(container._keystrings);
                ice::string::push_back(container._keystrings, key);

                // Add the new entry
                ice::array::push_back(container._entries, Config_v2::Key{ 0, CONFIG_KEYTYPE_STRING, CONFIG_VALTYPE_NONE, offset, ice::size(key) });

                // Return the entry
                result = ice::array::begin(container._entries) + idx;
            }

            return result;
        }

        auto get_entry(
            ice::Allocator& alloc,
            ice::ConfigBuilderValue::Entry& entry,
            ice::u32 idx,
            bool clean
        ) noexcept -> ice::ConfigBuilderValue::Entry*
        {
            ICE_ASSERT_CORE(entry.vtype == CONFIG_VALTYPE_TABLE);
            ice::ConfigBuilderContainer& container = *entry.data.val_container;

            ice::ConfigBuilderValue::Entry* result;
            if (idx < ice::count(container._entries))
            {
                result = ice::array::begin(container._entries) + idx;
                if (clean)
                {
                    clear_value_type(alloc, result);
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
                    ice::array::push_back(container._entries, Config_v2::Key{ 0, CONFIG_KEYTYPE_NONE, CONFIG_VALTYPE_NONE, 0, 0 });
                }

                // Return the entry
                result = ice::array::begin(container._entries) + idx;
            }

            return result;
        }

        auto get_value_entry(ice::ConfigBuilderContainer* container, ice::u32 ref) noexcept -> ice::ConfigBuilderValue::Entry*
        {
            // ice::ConfigBuilderValue::Entry* result = nullptr;
            return container->_entries._data + ref;
            // if (ref == ice::u32_max)
            // {
            //     result = static_cast<ConfigBuilderContainer*>(entry);
            // }
            // else
            // {
            //     result = entry->data.val_container->_entries._data + ref;
            // }
            // return result;
        }

    } // namespace detail

} // namespace ice
