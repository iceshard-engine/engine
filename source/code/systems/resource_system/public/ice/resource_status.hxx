#pragma once
#include <ice/base.hxx>

namespace ice
{

    enum class ResourceStatus : ice::u32
    {
        Invalid = 0x00'01,
        Available = 0x00'02,
        Loaded = 0x00'04,
        Loading = 0x01'00,
        Unloading = 0x02'00,
    };

} // namespace ice
