#pragma once
#include <core/memory.hxx>
#include <core/stack_string.hxx>

//! \brief The filesystem namespace.
namespace filesystem
{


//! \brief Initializes the file system.
void init(core::allocator& alloc, std::string_view basedir) noexcept;

//! \brief Shuts down the file system.
void shutdown() noexcept;

//! \brief Mounts the given directory available in the initialized filesystem.
void mount(std::string_view path) noexcept;


} // namespace filesystem
