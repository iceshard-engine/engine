#pragma once
#include <input/system.h>
#include <input/utils/message_pipe.h>
#include <input/utils/message_filter.h>

namespace mooned::io
{

class MessagePipe;

const MessagePipe& messages(IOSystem* system);

void clear_messages(IOSystem* system);

}
