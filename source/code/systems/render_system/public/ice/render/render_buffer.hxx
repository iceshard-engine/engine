#pragma once
#include <ice/base.hxx>

namespace ice::render
{

    enum class BufferType
    {
        Uniform,
        Vertex,
        Index,
        Transfer,
    };

    enum class Buffer : ice::uptr
    {
        Invalid = 0x0
    };

    struct BufferUpdateInfo
    {
        ice::render::Buffer buffer;
        ice::Data data;
    };

} // namespace ice::render
