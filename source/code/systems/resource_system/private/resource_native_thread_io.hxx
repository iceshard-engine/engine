/// Copyright 2023 - 2023, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/task.hxx>
#include <ice/task_thread.hxx>

namespace ice
{

    struct NativeIO;

    auto create_nativeio_thread_data(
        ice::Allocator& alloc,
        ice::ucount hint_thread_count = 1
    ) noexcept -> ice::UniquePtr<ice::NativeIO>;

    auto nativeio_handle(ice::NativeIO* nativeio) noexcept -> void*;

    auto nativeio_thread_procedure(ice::NativeIO* nativeio, ice::TaskQueue& queue) noexcept -> ice::u32;

} // namespace ice
