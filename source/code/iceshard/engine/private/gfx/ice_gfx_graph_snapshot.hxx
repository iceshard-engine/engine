#pragma once
#include <ice/base.hxx>

namespace ice::gfx
{

    enum GfxSnapshotEvent : ice::u8
    {
        EventInvalid = 0b0,
        EventReadRes = 0b0000'0001,
        EventWriteRes = 0b0000'0010,
        EventCreateRes = 0b0000'0100,
        EventDeleteRes = 0b0000'1000,

        EventBeginPass = 0b0011'0000,
        EventNextSubPass = 0b0001'0000,
        EventEndPass = 0b0100'0000,

        MaskResRW = EventWriteRes | EventReadRes,
        MaskRes = EventWriteRes | EventReadRes | EventCreateRes | EventDeleteRes,
        MaskPass = EventBeginPass | EventNextSubPass | EventEndPass
    };

    struct GfxGraphSnapshot
    {

        ice::u32 subpass;
        ice::gfx::GfxResource resource;
        ice::gfx::GfxSnapshotEvent event; // create, write, read, transfer
        ice::u32 info;

        static bool compare(GfxGraphSnapshot const& left, GfxGraphSnapshot const& right) noexcept
        {
            return (left.subpass < right.subpass)
                && ((left.event & MaskResRW) < (right.event & MaskResRW)
                    || (left.resource.value < right.resource.value));
        }
    };

} // namespace ice::gfx
