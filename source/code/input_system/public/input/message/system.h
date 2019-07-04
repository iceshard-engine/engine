#pragma once
#include <core/base.hxx>
#include <input/utils/message_info.h>

namespace input::message
{

//! Defines a empty tick message.
struct Tick { };

//! Defines a engine wide debug message, which can be anything
struct DebugMessage
{
    core::cexpr::stringid_type name;
    int64_t value;
};

//! Declares the given messages
DECLARE_MESSAGE(Tick);
DECLARE_MESSAGE(DebugMessage);

}
