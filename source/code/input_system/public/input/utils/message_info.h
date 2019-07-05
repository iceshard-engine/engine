#pragma once
#include <core/base.hxx>
#include <core/cexpr/stringid.hxx>
#include <type_traits>

namespace input::message
{

    //! \brief A message metadata object.
    struct Metadata
    {
        //! \brief The message type identifier.
        core::cexpr::stringid_type message_type;

        //! \brief The message timestamp.
        core::datetime::tick_type message_timestamp;
    };

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
