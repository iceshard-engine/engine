#pragma once
#include <ice/config_new.hxx>
#include <ice/mem_buffer.hxx>

namespace ice
{

    struct ConfigBuilder_v2
    {
        ConfigBuilder_v2(ice::Allocator& alloc) noexcept;

        Config_v2::Key* _keys;
        Config_v2::Value* _values;
        char const* _strings;
        void const* _data;

        operator Config_v2() const noexcept {
            return Config_v2{
                ._keys = _keys,
                ._values = _values,
                ._strings = _strings,
                ._data = _data
            };
        }

        ice::ucount _count;
        ice::ucount _capacity;
        ice::Buffer _keyval_buffer;
        ice::Buffer _keystr_buffer;
        ice::Buffer _data_buffer;
    };

    bool config_set_i16(ice::ConfigBuilder_v2& config, ice::String key, ice::i16 val) noexcept;

    bool config_set_u32(ice::ConfigBuilder_v2& config, ice::String key, ice::u32 val) noexcept;

    bool config_set_f64(ice::ConfigBuilder_v2& config, ice::String key, ice::f64 val) noexcept;

    bool config_set_str(ice::ConfigBuilder_v2& config, ice::String key, ice::String val) noexcept;

    bool config_remove(ice::ConfigBuilder_v2& config, ice::String key) noexcept;

    auto config_finalize(ice::ConfigBuilder_v2& config, ice::Allocator& alloc) noexcept -> ice::Memory;

} // namespace ice
