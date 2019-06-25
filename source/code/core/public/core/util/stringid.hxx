#pragma once
#include <kernel/compiletime/murmurhash.h>

#include <consthash/all.hxx>
#include <algorithm>

#ifdef _DEBUG
namespace mooned
{
    namespace detail
    {
        using stringid_hash_t = uint64_t;

        struct stringid_t
        {
            stringid_t(stringid_hash_t val, const char* str = "") : _val(val), _str() {
                memset(&_str[0], 0, sizeof(_str));
                memcpy(&_str[0], str, std::min(sizeof(_str) - 1, strlen(str)));
            }
            stringid_t(const stringid_t& o) : _val(o._val), _str() {
                memcpy(_str, o._str, sizeof(_str));
            }
            stringid_t(stringid_t&& o) : _val(o._val), _str() {
                memcpy(_str, o._str, sizeof(_str));
            }
            ~stringid_t() = default;

            stringid_t& operator=(const stringid_t& o) {
                if (&o == this) return *this;
                memcpy(_str, o._str, sizeof(_str));
                _val = o._val;
                return *this;
            }
            stringid_t& operator=(stringid_t&& o) {
                if (&o == this) return *this;
                memcpy(_str, o._str, sizeof(_str));
                _val = o._val;
                return *this;
            }

            bool operator==(const stringid_t& o) const {
                return _val == o._val;
            }

            bool operator!=(const stringid_t& o) const {
                return _val != o._val;
            }

            operator const char*() const { return _str; }
            operator stringid_hash_t() const { return _val; }

            stringid_hash_t _val;
            char _str[24];
        };

        static_assert(sizeof(stringid_t) == 32, "String ID size should be 32bytes long!");
    }
};

using stringid_t = mooned::detail::stringid_t;
using stringid_hash_t = mooned::detail::stringid_hash_t;

constexpr mooned::detail::stringid_hash_t _stringid(const char* str)
{
    return compiletime::hash_cstring(str, compiletime::length_cstring(str));
}

inline stringid_t stringid(const char* str)
{
    return stringid_t(_stringid(str), str);
}

inline stringid_t stringid(mooned::detail::stringid_hash_t id)
{
    return stringid_t(id, "");
}

#else
using stringid_t = uint64_t;
using stringid_hash_t = stringid_t;

constexpr stringid_t _stringid(const char* str)
{
    return compiletime::hash_cstring(str, compiletime::length_cstring(str));
}

constexpr stringid_t stringid(const char* str)
{
    return compiletime::hash_cstring(str, compiletime::length_cstring(str));
}
#endif

//! Returns a 32bit hash value for the given string.
//! \note Hash algorithm: Google CityHash 32bit.
inline uint32_t hash32(const char* str)
{
    return consthash::city32(str, compiletime::length_cstring(str));
}

//! Returns a 64bit hash value for the given string.
//! \note Hash algorithm: Google CityHash 64bit.
inline uint64_t hash64(const char* str)
{
    return consthash::city64(str, compiletime::length_cstring(str));
}

#ifdef _DEBUG
namespace std
{

template <> struct hash<stringid_t>
{
    size_t operator()(const stringid_t& x) const
    {
        return hash<mooned::detail::stringid_hash_t>()(x._val);
    }
};

template<>
struct is_pod<stringid_t> : std::integral_constant<stringid_hash_t, 1llu>
{
};

}
#endif
