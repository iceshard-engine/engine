#pragma once
#include <ice/base.hxx>

namespace ice
{

    enum class ResourceFlags : ice::u32
    {
        None,

        // First 4 bits are saved for types of targeted devices.
        Target_PC = 0x0000'0001,
        Target_Console = 0x0000'0002,
        Target_Handheld = 0x0000'0003,
        Target_Phone = 0x0000'0004,

        // Next 8 bits go to LOD from (0-7) 4 + 4 bits for future cases.
        LOD_0 = 0x0000'0010,
        LOD_1 = 0x0000'0020,
        LOD_2 = 0x0000'0030,
        LOD_3 = 0x0000'0040,
        LOD_4 = 0x0000'0050,
        LOD_5 = 0x0000'0060,
        LOD_6 = 0x0000'0070,
        LOD_Unused = 0x0000'0f00,

        // 20 bits to make use of. Please update this enum when this changes.
        First_UnusedValue = 0x0000'1000,

        // Masks to be used to help with flag reading.
        Mask_Targets = 0x0000'000F,
        Mask_LODs = 0x0000'0FF0,
    };

    static constexpr ice::ResourceFlags Constant_DefaultResourceFlags = []() noexcept -> ice::ResourceFlags
    {
        switch (ice::build::current_platform.system)
        {
        case ice::build::System::Unix: [[fallthrough]];
        case ice::build::System::UWP: [[fallthrough]];
        case ice::build::System::Windows:
            return ice::ResourceFlags::Target_PC;
        }
        return ice::ResourceFlags::None;
    }();

} // namespace ice
