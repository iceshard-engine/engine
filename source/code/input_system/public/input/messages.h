#pragma once
#include <input/system.h>
#include <input/utils/message_pipe.h>
#include <input/utils/message_filter.h>

namespace input
{

class MessageQueue;

const MessageQueue& messages(IOSystem* system);

void clear_messages(IOSystem* system);

}
