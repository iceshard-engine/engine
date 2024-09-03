#include <ice/config_builder.hxx>
#include <ice/container/hashmap.hxx>
#include <ice/string/heap_string.hxx>
#include <ice/string/heap_var_string.hxx>

#include "config_details.hxx"
#include "config_new.hxx"

namespace ice
{
    using Config_KeyType = ice::config::detail::KeyType;
    using Config_ValType = ice::config::detail::ValType;

    using enum ice::config::detail::KeyType;
    using enum ice::config::detail::ValType;

    struct ConfigBuilderContainer;

    namespace detail
    {

        auto key(ice::String src, ice::ConfigBuilderValue::Entry const& entry) noexcept -> ice::String;

        void assign_value_type(
            ice::Allocator& alloc,
            ice::ConfigBuilderValue::Entry& entry,
            ice::Config_ValType vtype
        ) noexcept;

        void assign_value_type(
            ice::Allocator& alloc,
            ice::ConfigBuilderValue::Entry& entry,
            ice::i32 vtype
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

        auto get_or_set_container(
            ice::Allocator& alloc,
            ice::ConfigBuilderValue::Entry& entry,
            ice::Config_ValType vtype
        ) noexcept -> ice::ConfigBuilderValue;

    } // namespace detail

    struct ConfigBuilderContainer;

    struct ConfigBuilderValue::Entry : ice::config::detail::ConfigKey
    {
        Entry(ice::config::detail::ConfigKey key) noexcept
            : ice::config::detail::ConfigKey{ key }
            , data{ .val_custom = { } }
        { }

        ~Entry() noexcept
        {
        }

        union Data
        {
            ice::u8 val_u8;
            ice::u16 val_u16;
            ice::u32 val_u32;
            ice::u64 val_u64;

            ice::i8 val_i8;
            ice::i16 val_i16;
            ice::i32 val_i32;
            ice::i64 val_i64;

            ice::f32 val_f32;
            ice::f64 val_f64;

            ice::HeapVarString<>* val_varstr;
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
            : Entry{ { .next = 0, .type = CONFIG_KEYTYPE_NONE, .vtype = vtype, .offset = 0, .size = 0 } }
            , _refcount{ 1 }
            , _entries{ alloc }
            , _keystrings{ alloc }
        {
            // Since we are a container we point to ourselves.
            data.val_container = this;
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
            detail::clear_value_type(*_alloc, ice::exchange(_internal, nullptr));
        }
    }

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
        detail::clear_value_type(*_alloc, _internal);
    }


    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

    template<>
    auto ConfigBuilderValue::set(ice::u8 value) noexcept -> ice::u8&
    {
        ConfigBuilderContainer::Entry* const entry = _idx == ice::u32_max
            ? _internal : (static_cast<ConfigBuilderContainer*>(_internal)->_entries._data + _idx);

        detail::assign_value_type(*_alloc, *entry, CONFIG_VALTYPE_8B_BIT);
        entry->data.val_u8 = value;
        return entry->data.val_u8;
    }

    template<>
    auto ConfigBuilderValue::set(ice::u16 value) noexcept -> ice::u16&
    {
        ConfigBuilderContainer::Entry* const entry = _idx == ice::u32_max
            ? _internal : (static_cast<ConfigBuilderContainer*>(_internal)->_entries._data + _idx);

        detail::assign_value_type(*_alloc, *entry, CONFIG_VALTYPE_16B_BIT);
        entry->data.val_u16 = value;
        return entry->data.val_u16;
    }

    template<>
    auto ConfigBuilderValue::set(ice::u32 value) noexcept -> ice::u32&
    {
        ConfigBuilderContainer::Entry* const entry = _idx == ice::u32_max
            ? _internal : (static_cast<ConfigBuilderContainer*>(_internal)->_entries._data + _idx);

        detail::assign_value_type(*_alloc, *entry, CONFIG_VALTYPE_32B_BIT);
        entry->data.val_u32 = value;
        return entry->data.val_u32;
    }

    template<>
    auto ConfigBuilderValue::set(ice::i8 value) noexcept -> ice::i8&
    {
        ConfigBuilderContainer::Entry* const entry = _idx == ice::u32_max
            ? _internal : (static_cast<ConfigBuilderContainer*>(_internal)->_entries._data + _idx);

        detail::assign_value_type(*_alloc, *entry, CONFIG_VALTYPE_8B_BIT | CONFIG_VALTYPE_SIGN_BIT);
        entry->data.val_i8 = value;
        return entry->data.val_i8;
    }

    template<>
    auto ConfigBuilderValue::set(ice::i16 value) noexcept -> ice::i16&
    {
        ConfigBuilderContainer::Entry* const entry = _idx == ice::u32_max
            ? _internal : (static_cast<ConfigBuilderContainer*>(_internal)->_entries._data + _idx);

        detail::assign_value_type(*_alloc, *entry, CONFIG_VALTYPE_16B_BIT | CONFIG_VALTYPE_SIGN_BIT);
        entry->data.val_i16 = value;
        return entry->data.val_i16;
    }

    template<>
    auto ConfigBuilderValue::set(ice::i32 value) noexcept -> ice::i32&
    {
        ConfigBuilderContainer::Entry* const entry = _idx == ice::u32_max
            ? _internal : (static_cast<ConfigBuilderContainer*>(_internal)->_entries._data + _idx);

        detail::assign_value_type(*_alloc, *entry, CONFIG_VALTYPE_32B_BIT | CONFIG_VALTYPE_SIGN_BIT);
        entry->data.val_u32 = value;
        return entry->data.val_i32;
    }

    template<>
    auto ConfigBuilderValue::set(ice::f32 value) noexcept -> ice::f32&
    {
        ConfigBuilderContainer::Entry* const entry = _idx == ice::u32_max
            ? _internal : (static_cast<ConfigBuilderContainer*>(_internal)->_entries._data + _idx);

        detail::assign_value_type(*_alloc, *entry, CONFIG_VALTYPE_32B_BIT | CONFIG_VALTYPE_SIGN_BIT | CONFIG_VALTYPE_FP_BIT);
        entry->data.val_f32 = value;
        return entry->data.val_f32;
    }

    template<>
    auto ConfigBuilderValue::set(ice::f64 value) noexcept -> ice::f64&
    {
        ConfigBuilderContainer::Entry* const entry = _idx == ice::u32_max
            ? _internal : (static_cast<ConfigBuilderContainer*>(_internal)->_entries._data + _idx);

        detail::assign_value_type(*_alloc, *entry, CONFIG_VALTYPE_64B_BIT | CONFIG_VALTYPE_SIGN_BIT | CONFIG_VALTYPE_FP_BIT);
        entry->data.val_f64 = value;
        return entry->data.val_f64;
    }

    auto ConfigBuilderValue::set(ice::String value) noexcept -> ice::HeapVarString<>&
    {
        ConfigBuilderContainer::Entry* const entry = _idx == ice::u32_max
            ? _internal : (static_cast<ConfigBuilderContainer*>(_internal)->_entries._data + _idx);

        detail::assign_value_type(*_alloc, *entry, CONFIG_VALTYPE_STRING);

        entry->data.val_varstr = _alloc->create<ice::HeapVarString<>>(*_alloc, value);
        return *entry->data.val_varstr;
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
                ice::ucount bytes = 0;
                ice::ucount const size = ice::string::detail::read_varstring_size(entry->data.val_varstr->_data, bytes);
                out_data_size += { bytes + size + 1 }; // +1 for '\0'
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
        ice::config::detail::ConfigKey* out_keylist,
        ice::config::detail::ConfigValue* out_vallist,
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
                ice::HeapVarString<> const& varstr = *config._entries[out_keyidx].data.val_varstr;
                ice::Data const in_data = ice::string::data_view(varstr);

                // Set the '\0' character
                *(reinterpret_cast<char*>(out_data.memory.location) + out_data.offset_strings) = '\0';

                // Update strings offset (excluding '\0')
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

    using ice::config::detail::ConfigKey;
    using ice::config::detail::ConfigValue;

    auto ConfigBuilder::finalize(ice::Allocator& alloc) noexcept -> ice::Memory
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
        ice::ConfigKey* new_keys = reinterpret_cast<ice::ConfigKey*>(final_buffer.location);

        // First key is the root key (It has no value attached!)
        new_keys[0] = ice::ConfigKey{ .next = 1, .type = CONFIG_KEYTYPE_NONE, .vtype = CONFIG_VALTYPE_ROOT, .offset = final_count >> 8, .size = final_count & 0xff };

        ice::ConfigValue* new_values = reinterpret_cast<ice::ConfigValue*>(new_keys + final_count + 2);

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
                ice::array::push_back(container._entries, ConfigKey{ 0, CONFIG_KEYTYPE_STRING, CONFIG_VALTYPE_NONE, offset, ice::size(key) });

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
                    ice::array::push_back(container._entries, ConfigKey{ 0, CONFIG_KEYTYPE_NONE, CONFIG_VALTYPE_NONE, 0, 0 });
                }

                // Return the entry
                result = ice::array::begin(container._entries) + idx;
            }

            return result;
        }

    } // namespace detail

} // namespace ice
