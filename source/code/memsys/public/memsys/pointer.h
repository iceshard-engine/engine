#pragma once
#include "memsys/allocator.h"

#include <utility>
#include <memory>

namespace mem
{
namespace detail
{

//! A default deleter for our classes.
template<class T>
class mooned_deleter
{
public:
    //! A default deleter has a nullptr as an allocator.
    mooned_deleter() noexcept
        : _allocator{ nullptr }
    { }

    //! A deleter needs the allocator used while creating the object.
    mooned_deleter(mem::allocator& alloc) noexcept
        : _allocator{ &alloc }
    { }

    //! Allow moving while the allocator object is the same.
    mooned_deleter(mooned_deleter&& other) noexcept
        : _allocator{ other._allocator }
    { }

    mooned_deleter& operator=(mooned_deleter&& other) noexcept
    {
        if (this == &other) return *this;
        _allocator = other._allocator;
        other._allocator = nullptr;
        return *this;
    }

    //! operator() to make it possible to use with std::unique_ptr
    void operator()(T* object) noexcept
    {
        MAKE_DELETE((*_allocator), T, object);
    }

private:
    mem::allocator* _allocator;
};

} // namespace detail

//! We use the unique_ptr implementation.
template<class T>
using unique_pointer = std::unique_ptr<T, detail::mooned_deleter<T>>;

//! Our own make_unique implementation.
template<class T, class... Args>
unique_pointer<T> make_unique(mem::allocator& alloc, Args&&... args)
{
    return { MAKE_NEW(alloc, T, std::forward<Args>(args)...), detail::mooned_deleter<T>{ alloc } };
}

} // namespace mem
