#pragma once
#include <ice/mem_allocator.hxx>
#include <ice/resource_tracker.hxx>

extern ice::i32 game_main(
    ice::Allocator& game_allocator,
    ice::ResourceTracker& resources
);
