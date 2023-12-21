/// Copyright 2023 - 2023, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#include "iceshard_frame.hxx"

namespace ice
{

    IceshardEngineFrame::IceshardEngineFrame(ice::IceshardFrameData& frame_data) noexcept
        : _data{ frame_data }
        , _shards{ _data._allocator }
        , _operations{ frame_data._allocator, 16 }
    {
    }

    auto create_iceshard_frame(
        ice::Allocator& alloc,
        ice::EngineFrameData& frame_data,
        ice::EngineFrameFactoryUserdata
    ) noexcept -> ice::UniquePtr<ice::EngineFrame>
    {
        return ice::make_unique<ice::IceshardEngineFrame>(alloc, static_cast<ice::IceshardFrameData&>(frame_data));
    }

} // namespace ice
