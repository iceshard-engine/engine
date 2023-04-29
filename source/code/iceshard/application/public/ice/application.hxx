/// Copyright 2022 - 2022, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/mem_allocator.hxx>
#include <ice/resource_tracker.hxx>
#include <ice/task_types.hxx>

extern ice::i32 game_main(
    ice::Allocator& game_allocator,
    ice::TaskScheduler& scheduler,
    ice::ResourceTracker& resources
);
