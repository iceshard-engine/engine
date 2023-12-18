#include "iceshard_frame_v2.hxx"

namespace ice
{

    IceshardEngineFrame::IceshardEngineFrame(ice::IceshardFrameData& frame_data) noexcept
        : _data{ frame_data }
        , _shards{ _data._allocator }
    {
    }

} // namespace ice::v2
