#pragma once
#include <memsys/allocator.hxx>

namespace memsys
{

//! \brief Global allocator implementation details.
namespace detail
{

//! \brief Header for all global memory allocations.
struct global_memory_header
{

uint32_t allocation_size;

};

} // namespace detail

//! \brief The allocator used for global allocations.
//! \details Calls directly the build in 'malloc' and 'free' functions.
class global_allocator : public memsys::allocator
{
public:
    global_allocator() noexcept;
    virtual ~global_allocator() noexcept;
};


} // namespace memsys
