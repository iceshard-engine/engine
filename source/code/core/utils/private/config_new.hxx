#pragma once
#include <ice/config_new.hxx>

namespace ice
{

    enum Config_KeyType : ice::u8
    {
        CONFIG_KEYTYPE_NONE = 0,
        CONFIG_KEYTYPE_STRING = 1,
    };

    enum Config_ValType : ice::u8
    {
        CONFIG_VALTYPE_NONE = 0x00,

        CONFIG_VALTYPE_8B_BIT = 0x01,
        CONFIG_VALTYPE_16B_BIT = 0x02,
        CONFIG_VALTYPE_32B_BIT = 0x03,
        CONFIG_VALTYPE_SIGN_BIT = 0x04,
        CONFIG_VALTYPE_FP_BIT = 0x08,

        CONFIG_VALTYPE_LARGE = 0x10,
        CONFIG_VALTYPE_64B_BIT = CONFIG_VALTYPE_LARGE,

        // Other types that should be checked directly
        CONFIG_VALTYPE_OTHER = 0x20,
        CONFIG_VALTYPE_STRING = CONFIG_VALTYPE_OTHER,

        // User provided types
        CONFIG_VALTYPE_USER = 0x30,

        CONFIG_VALTYPE_CONTAINER = 0x3C, // Last 4 values 60, 61, 62, 63
        CONFIG_VALTYPE_TABLE = CONFIG_VALTYPE_CONTAINER + 0,
        CONFIG_VALTYPE_OBJECT = CONFIG_VALTYPE_CONTAINER + 1,
        CONFIG_VALTYPE_ROOT = CONFIG_VALTYPE_CONTAINER + 3,
    };

    struct Config_v2::Key
    {
        ice::u32 next : 1;
        ice::u32 type : 1; // 1=Strings, 0=None (Used By Arrays)
        ice::u32 vtype : 6;

        ice::u32 offset : 16;
        ice::u32 size : 8;

        static auto key_str(Key key, void const* data) noexcept -> ice::String
        {
            ICE_ASSERT_CORE(key.type == CONFIG_KEYTYPE_STRING);
            return ice::String{ reinterpret_cast<char const*>(data) + key.offset, key.size };
        }
    };

    struct Config_v2::Value
    {
        ice::u32 internal;
    };

    inline auto read_varstring_size(void const* data, ice::u32& out_bytes) noexcept -> ice::u32
    {
        ice::u32 result = 0;
        ice::u8 const* varb = reinterpret_cast<ice::u8 const*>(data);
        out_bytes = 1;
        while(*varb & 0x80)
        {
            result += *varb;
            result <<= 7;
            varb += 1;
            out_bytes += 1;
        }
        return result + *varb;
    }

    inline auto write_varstring_size(void* data, ice::u32 size) noexcept -> ice::u32
    {
        ice::u32 bytes = 0;
        ice::u8* varb = reinterpret_cast<ice::u8*>(data);
        while(size > 0x7f)
        {
            varb[bytes] = (size & 0x7f) | 0x80;
            size >>= 7;
            bytes += 1;
        }
        varb[bytes] = size & 0x7f;
        return bytes + 1;
    }

} // namespace ice
