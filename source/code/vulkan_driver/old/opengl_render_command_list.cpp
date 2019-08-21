#include <renderlib/render_commands.h>

#include <kernel/compiletime/stringid.h>

const char* tostring(const mooned::render::Command& cmd)
{
#define RENDER_COMMAND(name) case _stringid(#name): return #name
    switch (cmd.id)
    {
#include "opengl_render_command_list.h"
    }
#undef RENDER_COMMAND
    return "<invalid_command>";
}

