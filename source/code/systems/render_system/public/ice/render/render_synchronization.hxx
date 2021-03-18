#pragma once
#include <ice/base.hxx>

namespace ice::render
{

    enum class Semaphore : ice::uptr
    {
        Invalid = 0x0
    };

    enum class Fence : ice::uptr
    {
        Invalid = 0x0
    };

} // namespace ice::render
