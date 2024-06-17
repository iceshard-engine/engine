/// Copyright 2023 - 2023, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/resource.hxx>
#include <ice/task.hxx>

#include "native/native_aio_tasks.hxx"

namespace ice
{

    class FileSystemResource : public ice::Resource
    {
    public:
        virtual auto size() const noexcept -> ice::usize = 0;

        virtual auto load_data(
            ice::Allocator& alloc,
            ice::TaskScheduler& scheduler,
            ice::NativeAIO* nativeio
        ) const noexcept -> ice::Task<ice::Memory> = 0;
    };

} // namespace ice
