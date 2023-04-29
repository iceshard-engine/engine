/// Copyright 2022 - 2022, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/task.hxx>
#include <ice/task_types.hxx>
#include <ice/resource.hxx>

#include "../resource_native_thread_io.hxx"

#if ISP_WINDOWS

namespace ice
{

    class Resource_Win32 : public ice::LooseResource
    {
    public:
        virtual auto load_data(
            ice::Allocator& alloc,
            ice::TaskScheduler& scheduler,
            ice::NativeIO* nativeio
        ) const noexcept -> ice::Task<ice::Memory> = 0;
    };

} // namespace ice

#endif // #if ISP_WINDOWS
