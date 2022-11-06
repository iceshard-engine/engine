/// Copyright 2022 - 2022, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/task.hxx>
#include <ice/resource.hxx>

#if ISP_WINDOWS

namespace ice
{

    class TaskScheduler_v2;

    class Resource_Win32 : public ice::LooseResource
    {
    public:
        virtual auto load_data_for_flags(
            ice::Allocator& alloc,
            ice::ResourceFlags flags,
            ice::TaskScheduler_v2& scheduler
        ) const noexcept -> ice::Task<ice::Memory> = 0;
    };

} // namespace ice

#endif // #if ISP_WINDOWS
