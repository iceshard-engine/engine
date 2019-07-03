#pragma once
#include <iolib/system.h>
#include <iolib/utils/message_pipe.h>
#include <iolib/utils/message_filter.h>

namespace mooned::io
{

class MessagePipe;

const MessagePipe& messages(IOSystem* system);

void clear_messages(IOSystem* system);

}
