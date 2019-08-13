#pragma once
#include <core/allocator.hxx>
#include <resource/system.hxx>

//! \brief The main entry point for a game.
//!
//! \details This function should be used as the main entry point for all games build with this engine.
//!
//! \param[in] game_allocator The main allocator object, which should be used to allocate all other objects.
int game_main(core::allocator& game_allocator, resource::ResourceSystem& resources);
