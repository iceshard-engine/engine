/// Copyright 2023 - 2023, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/task.hxx>
#include <ice/task_thread.hxx>

namespace ice
{

    struct NativeAIO;

    auto create_nativeio_thread_data(
        ice::Allocator& alloc,
        ice::TaskScheduler& default_scheduler,
        ice::ucount hint_thread_count = 1
    ) noexcept -> ice::UniquePtr<ice::NativeAIO>;

    auto nativeio_handle(ice::NativeAIO* nativeio) noexcept -> void*;

    auto nativeio_thread_procedure(
        ice::NativeAIO* nativeio,
        ice::TaskQueue& queue
    ) noexcept -> ice::u32;

} // namespace ice
