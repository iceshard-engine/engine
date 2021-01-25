#pragma once
#include <ice/allocator.hxx>
#include <ice/resource_system.hxx>

extern ice::i32 game_main(
    ice::Allocator& game_allocator,
    ice::ResourceSystem& resources
);
