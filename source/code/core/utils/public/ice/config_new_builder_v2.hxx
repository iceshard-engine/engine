#pragma once
#include <ice/config_new.hxx>
#include <ice/container/array.hxx>
#include <ice/string/heap_string.hxx>

namespace ice
{

    struct ConfigBuilderValue
    {
        struct Entry;

        ConfigBuilderValue(ice::Allocator* alloc, Entry* entry, ice::u32 ref) noexcept;
        ~ConfigBuilderValue() noexcept;

        ConfigBuilderValue(ConfigBuilderValue&&) noexcept;
        ConfigBuilderValue(ConfigBuilderValue const&) noexcept;
        auto operator=(ConfigBuilderValue&&) noexcept -> ConfigBuilderValue&;
        auto operator=(ConfigBuilderValue const&) noexcept -> ConfigBuilderValue&;

        auto operator[](ice::String key) noexcept -> ConfigBuilderValue;
        auto operator[](ice::u32 idx) noexcept -> ConfigBuilderValue;

        template<typename T> requires std::is_trivial_v<T>
        auto operator=(T value) noexcept -> T&;

        template<typename T>
        void set(T value) noexcept;

        template<typename T>
        auto operator=(T value) noexcept
        {
            return set(value);
        }

        void reset();

        ice::Allocator* _alloc;
        Entry* _internal;
        ice::u32 _idx;
    };

    class ConfigBuilder_v2 : public ConfigBuilderValue
    {
    public:
        ConfigBuilder_v2(ice::Allocator& alloc) noexcept;
        ~ConfigBuilder_v2() noexcept;

        auto finalize(ice::Allocator& alloc) noexcept -> ice::Memory;
    };

    // auto create_config_builder(ice::Allocator& alloc) noexcept -> ConfigBuilder_v2;

    struct VarString
    {
        char const* ptr;

        bool operator==(ice::String str) const noexcept;
        operator ice::String() const noexcept;
    };

    struct ConfigBuilder
    {

        struct Entry
        {
            ice::u32 next : 1;
            ice::u32 type : 1; // 1=Strings, 0=None (Used By Arrays)
            ice::u32 vtype : 6;

            ice::u32 offset : 16;
            ice::u32 size : 8;

            union Data
            {
                ice::u32 val_u32;
                ice::f32 val_f32;
                ice::u64 val_u64;
                ice::i64 val_i64;
                ice::f64 val_f64;
                ice::VarString val_varstr;
                ice::ConfigBuilder* sub;
            } data;

            static auto key(ice::String src, Entry const& entry) noexcept -> ice::String
            {
                return { src._data + entry.offset, entry.size };
            }
        };

        struct Value
        {
            Allocator* alloc;
            Entry* entry;

            auto operator[](ice::String key) noexcept -> Value;

            template<typename T>
            void set(T value) noexcept;

            template<typename T>
            auto operator=(T value) noexcept
            {
                return set(value);
            }
        };

        ConfigBuilder(ice::Allocator& alloc, ConfigType type) noexcept;
        ~ConfigBuilder() noexcept;

        auto get_entry(ice::u32 idx = ice::u32_max, bool clean = true) noexcept -> ConfigBuilder::Entry&;
        auto get_entry(ice::String key, bool clean = true) noexcept -> ConfigBuilder::Entry&;

        auto new_object(ice::String key = "") noexcept -> ConfigBuilder&;
        auto new_table(ice::String key = "") noexcept -> ConfigBuilder&;

        void set_u32(ice::String key, ice::u32 val) noexcept;
        void set_u64(ice::String key, ice::u64 val) noexcept;

        void add_f32(ice::f32 val) noexcept;
        void add_u8(ice::u8 val) noexcept;


        auto finalize(ice::Allocator& alloc) noexcept -> ice::Memory;

        auto operator[](ice::String key) noexcept -> Value;

        ice::ConfigType const _type;
        ice::Allocator& _allocator;
        ice::Array<Entry> _entries;
        ice::HeapString<> _keystrings;
    };

    template<> void ConfigBuilder::Value::set(ice::i32 value) noexcept;
    template<> void ConfigBuilder::Value::set(ice::u32 value) noexcept;

    inline ConfigBuilder::ConfigBuilder(ice::Allocator& alloc, ConfigType type) noexcept
        : _type{ type }
        , _allocator{ alloc }
        , _entries{ _allocator }
        , _keystrings{ _allocator }
    {
    }

    inline ConfigBuilder::~ConfigBuilder() noexcept
    {
        for (Entry const& entry : _entries)
        {
            if (entry.vtype >= 60)
            {
                _allocator.destroy(entry.data.sub);
            }
        }
    }

    inline auto ConfigBuilder::get_entry(ice::u32 idx, bool clean) noexcept -> ConfigBuilder::Entry&
    {
        ICE_ASSERT_CORE(_type == ConfigType::Table);

        if (idx < ice::count(_entries))
        {
            if (_entries[idx].vtype >= 60) // is allocated?
            {
                _allocator.destroy(_entries[idx].data.sub);
            }
            _entries[idx].vtype = 0;
        }
        else
        {
            if (idx == ice::u32_max)
            {
                idx = ice::count(_entries);
            }

            while(ice::count(_entries) <= idx)
            {
                ice::array::push_back(_entries, { 0, 0, 0, 0, 0, Entry::Data{ } });
            }
        }

        return _entries[idx];
    }


    inline auto ConfigBuilder::get_entry(ice::String key, bool clean) noexcept -> ConfigBuilder::Entry&
    {
        ICE_ASSERT_CORE(_type == ConfigType::Object);

        ice::u32 idx = 0;
        for (;idx < ice::count(_entries); ++idx)
        {
            ice::String const entry_key = Entry::key(_keystrings, _entries[idx]);
            if (key == entry_key)
            {
                break;
            }
        }

        if (idx < ice::count(_entries))
        {
            if (_entries[idx].vtype >= 60) // is allocated?
            {
                _allocator.destroy(_entries[idx].data.sub);
            }
            _entries[idx].vtype = 0;
        }
        else
        {
            idx = ice::count(_entries);
            ice::u32 const offset = ice::string::size(_keystrings);
            ice::string::push_back(_keystrings, key);

            // TODO Create key value
            ice::array::push_back(_entries, { 0, 1, 0, offset, ice::size(key), Entry::Data{ } });
        }

        return _entries[idx];
    }

    inline auto ConfigBuilder::new_object(ice::String key) noexcept -> ConfigBuilder&
    {
        Entry& entry = _type == ConfigType::Object ? get_entry(key, true) : get_entry(ice::u32_max, true);
        entry.vtype = 61;
        entry.data.sub = _allocator.create<ConfigBuilder>(_allocator, ConfigType::Object);
        return *entry.data.sub;
    }

    inline auto ConfigBuilder::new_table(ice::String key) noexcept -> ConfigBuilder&
    {
        Entry& entry = _type == ConfigType::Object ? get_entry(key, true) : get_entry(ice::u32_max, true);
        entry.vtype = 60;
        entry.data.sub = _allocator.create<ConfigBuilder>(_allocator, ConfigType::Table);
        return *entry.data.sub;
    }

    inline auto configbuilder_root(ConfigBuilder& b, ice::Allocator& alloc) noexcept -> ice::Memory
    {
#if 1
        ConfigBuilder_v2 cb{ alloc };
        cb["asd"][0][1];
        ConfigBuilderValue val = cb["asd"];
        cb["a"];
        cb["b"];
        cb["c"];
        cb["d"];
        val = cb["asd"][0];//[0][1]["ad"];
        cb.reset();
        val = cb["e"][3];
        cb["e"][4];
        cb["f"];

        ice::Memory mem = cb.finalize(alloc);
        alloc.deallocate(mem);
        return {};
#else
        ConfigBuilder& o0 = b.new_object("a");
        [[maybe_unused]]
        ConfigBuilder& o1 = o0.new_object("b");
        [[maybe_unused]]
        ConfigBuilder& o2c = o0.new_table("c");
        o2c.add_f32(0.2f);
        o2c.add_f32(3.14f);
        o2c.add_f32(-0.69f);
        o2c.add_u8(69);
        ConfigBuilder& o2 = o2c.new_table().new_object();
        o2.set_u32("test", 42069);
        // o1.set_u32("g", 123);
        // o1.set_u32("f", 69);
        // o2.set_u32("g", 44123);
        // o2.set_u32("f", 4469);
        // o2.set_u64("d", ice::u64_max);
        b["my"]["holy"]["cow"] = 3u;

        ice::Memory result = b.finalize(alloc);
        ice::Config_v2 c = ice::config_load_v2(ice::data_view(result));

        [[maybe_unused]]
        ice::Config_v2 ca = ice::config_get(c, "a.c");
        [[maybe_unused]]
        ice::Config_v2 cb = ice::config_get(c, "a.b");
        ice::u32 rc = 0;
        ice::config_get_u32(c, "a.b", rc);

        ice::u8 u8a = 0;
        ice::u32 f0 = {};
        ice::config_get_u32(c, "my.holy.cow", f0);
        ice::config_get_u8(ca, 2, u8a);
        rc = 0;

        alloc.deallocate(result);
        return result;
#endif
    }

} // namespace ice
