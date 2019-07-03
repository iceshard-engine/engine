#pragma once
#include <kernel/types.h>
#include <iolib/utils/message_info.h>

namespace mooned::io::message
{

//! Defines a empty tick message.
struct Tick { };

//! Defines a engine wide debug message, which can be anything
struct DebugMessage
{
    stringid_hash_t name;
    int64_t value;
};

//! Declares the given messages
DECLARE_MESSAGE(Tick);
DECLARE_MESSAGE(DebugMessage);

}
