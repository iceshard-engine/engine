#pragma once
#include <ice/config_new.hxx>
#include <ice/container/array.hxx>
#include <ice/string/heap_string.hxx>

namespace ice
{

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
            ice::u32 key;
            ice::u32 keyval;
            union Data
            {
                ice::u64 val_u64;
                ice::i64 val_i64;
                ice::f64 val_f64;
                ice::VarString val_varstr;
                ice::ConfigBuilder* sub;
            } data;
        };

        ConfigBuilder(ice::Allocator& alloc, ConfigType type) noexcept;

        auto new_object(ice::String key = "") noexcept -> ConfigBuilder&;

        ice::ConfigType const _type;
        ice::Allocator& _allocator;
        ice::Array<Entry> _entries;
        ice::HeapString<> _keystrings;
    };

    ConfigBuilder::ConfigBuilder(ice::Allocator& alloc, ConfigType type) noexcept
        : _type{ type }
        , _allocator{ alloc }
        , _entries{ _allocator }
        , _keystrings{ _allocator }
    {
    }

    inline auto ConfigBuilder::new_object(ice::String key) noexcept -> ConfigBuilder&
    {
        ICE_ASSERT_CORE(_type == ConfigType::Object);

        ice::u32 idx = 0;
        for (;idx < ice::count(_entries); ++idx)
        {
            ice::String const entry_key = ice::string::substr(_keystrings, _entries[idx].keyval, 0);
            if (key == entry_key)
            {
                break;
            }
        }

        ConfigBuilder* new_obj = _allocator.create<ConfigBuilder>(_allocator, ConfigType::Object);
        if (idx < ice::count(_entries))
        {
            if (_entries[idx].key == 0) // is allocated?
            {
                // TODO: delete
                // _allocator.destroy(_entries[idx].data.sub, new_obj);
            }

            _entries[idx].data.sub = new_obj;
        }
        else
        {
            ice::u32 const offset = ice::string::size(_keystrings);
            ice::string::push_back(_keystrings, key);

            // TODO Create key value
            ice::array::push_back(_entries, { 0, offset, Entry::Data{ .sub = new_obj } });
        }
        return *new_obj;
        // TODO: insert return statement here
    }

} // namespace ice
