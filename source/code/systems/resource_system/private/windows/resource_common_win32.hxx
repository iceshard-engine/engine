/// Copyright 2022 - 2023, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/task.hxx>
#include <ice/task_types.hxx>
#include <ice/resource.hxx>

#if ISP_WINDOWS
#include "native/native_aio.hxx"

namespace ice
{

    class Resource_Win32 : public ice::LooseResource
    {
    public:
        virtual auto load_data(
            ice::Allocator& alloc,
            ice::TaskScheduler& scheduler,
            ice::NativeAIO* nativeio
        ) const noexcept -> ice::Task<ice::Memory> = 0;
    };

} // namespace ice

#endif // #if ISP_WINDOWS
