#include <renderlib/render_commands.h>
#include <memsys/memsys.h>

#include <cassert>

mooned::render::CommandBuffer::CommandBuffer(mem::allocator& alloc)
    : _allocator{ alloc }
    , _commands{ alloc }
    , _data{ alloc }
{
    pod::array::set_capacity(_commands, 40); // 40 commands for a good start (10 cache lines?)
    pod::buffer::set_capacity(_data, 1 * 1024); // 1 KB
}

mooned::render::CommandBuffer::~CommandBuffer()
{
    clear();
}

bool mooned::render::CommandBuffer::empty() const
{
    return pod::array::empty(_commands);
}

void mooned::render::CommandBuffer::clear()
{
    pod::array::clear(_commands);
    pod::buffer::clear(_data);
}

void* mooned::render::CommandBuffer::append(stringid_hash_t id, uint32_t size)
{
    if (size % mem::allocator::DEFAULT_ALIGN != 0u)
    {
        auto diff = mem::allocator::DEFAULT_ALIGN - static_cast<int>(size % mem::allocator::DEFAULT_ALIGN);
        assert(diff > 0);
        size += static_cast<unsigned>(diff);
    }

    // get the current data pos and allocate enough to hold the requested size
    uint32_t used = pod::buffer::size(_data);
    pod::buffer::resize(_data, used + size);

    // Save the command
    pod::array::push_back(_commands, Command{ id, size, used });

    // Return the pointer to the requested location
    auto* ptr = command_data(pod::array::end(_commands) - 1);

    // Check the calculated pointer and return it to the caller, it's his task to not overflow over it.
    assert((mem::utils::pointer_distance(pod::buffer::data(_data), ptr) + size) == pod::buffer::size(_data));
    return const_cast<void*>(ptr);
}

const mooned::render::Command* mooned::render::CommandBuffer::begin() const
{
    return pod::array::begin(_commands);
}

const mooned::render::Command* mooned::render::CommandBuffer::end() const
{
    return pod::array::end(_commands);
}

const void* mooned::render::CommandBuffer::command_data(const Command* cmd) const
{
    // Return the pointer to the requested location
    return mem::utils::pointer_add(pod::buffer::data(_data), cmd->data_offset);
}
