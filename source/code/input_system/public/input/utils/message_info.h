#pragma once
#include <core/base.hxx>
#include <core/cexpr/stringid.hxx>
#include <type_traits>

namespace input::message
{

template<class T>
struct MessageInfo;

#ifdef DECLARE_MESSAGE
#error "'DEFINE_MESSAGE' macro already defined!""
#else
#define DECLARE_MESSAGE(name) \
    static_assert(std::is_pod<##name>::value, "Message " #name " is not a POD type!"); \
    template<> struct MessageInfo<##name> { \
        static constexpr std::string_view Name = #name;\
        static constexpr auto ID = core::cexpr::stringid_cexpr(#name);\
    };
#endif

}
