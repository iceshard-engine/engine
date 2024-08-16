#pragma once
#include <ice/string/string.hxx>

namespace ice
{

    enum class ConfigType : ice::u8
    {
        Root,
        Table,
        Object,
        Value,
    };

    struct Config_v2
    {
        struct Key;
        struct Value;

        Key const* _keys;
        Value const* _values;
        char const* _strings;
        void const* _data;
    };

    auto config_type(ice::Config_v2 const& config) noexcept -> ice::ConfigType;
    auto config_get(ice::Config_v2 const& config, ice::u32 idx) noexcept -> ice::Config_v2;
    auto config_get(ice::Config_v2 const& config, ice::String key) noexcept -> ice::Config_v2;

    bool config_get_i16(ice::Config_v2 const& config, ice::String key, ice::i16& out_val) noexcept;
    bool config_get_u32(ice::Config_v2 const& config, ice::String key, ice::u32& out_val) noexcept;
    bool config_get_u64(ice::Config_v2 const& config, ice::String key, ice::u64& out_val) noexcept;
    bool config_get_f32(ice::Config_v2 const& config, ice::String key, ice::f32& out_val) noexcept;
    bool config_get_f64(ice::Config_v2 const& config, ice::String key, ice::f64& out_val) noexcept;

    bool config_get_u8(ice::Config_v2 const& config, ice::u32 idx, ice::u8& out_val) noexcept;
    bool config_get_f32(ice::Config_v2 const& config, ice::u32 idx, ice::f32& out_val) noexcept;

    bool config_get_str(ice::Config_v2 const& config, ice::String key, ice::String& out_val) noexcept;

    auto config_load_v2(ice::Data data) noexcept -> ice::Config_v2;

} // namespace ice
