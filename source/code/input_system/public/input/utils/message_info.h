#pragma once
#include <kernel/types.h>
#include <kernel/compiletime/stringid.h>
#include <type_traits>

namespace mooned::io::message
{

template<class T>
struct MessageInfo;

#ifdef DECLARE_MESSAGE
#error "'DEFINE_MESSAGE' macro already defined!""
#else
#define DECLARE_MESSAGE(name) \
    static_assert(std::is_pod<##name>::value, "Message " #name " is not a POD type!"); \
    template<> struct MessageInfo<##name> { \
        static constexpr const char* Name = #name;\
        static constexpr auto ID = _stringid(#name);\
    };
#endif

}
