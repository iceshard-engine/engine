#pragma once
#include <ice/base.hxx>

namespace ice::render
{

    enum class Buffer : ice::uptr
    {
        Invalid = 0x0
    };

    enum class BufferType : ice::u32
    {
        Uniform,
        Vertex,
        Index,
        Transfer,
    };

    struct BufferUpdateInfo
    {
        ice::render::Buffer buffer;
        ice::Data data;
        ice::u32 offset;
    };

} // namespace ice::render
