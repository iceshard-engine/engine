
template<class T>
inline core::memory::detail::memsys_deleter<T>::memsys_deleter(core::allocator& alloc) noexcept
    : _allocator{ &alloc }
{ }

template<class T>
inline core::memory::detail::memsys_deleter<T>::memsys_deleter(memsys_deleter&& other) noexcept
    : _allocator{ other._allocator }
{ }

template<class T>
inline auto core::memory::detail::memsys_deleter<T>::operator=(memsys_deleter&& other) noexcept -> memsys_deleter&
{
    if (this == &other) return *this;
    _allocator = other._allocator;
    other._allocator = nullptr;
    return *this;
}

template<class T>
inline void core::memory::detail::memsys_deleter<T>::operator()(T* object) noexcept
{
    MAKE_DELETE((*_allocator), T, object);
}

template<class Result, class Type, class... Args>
inline auto core::memory::make_unique(core::allocator& alloc, Args&&... args) noexcept -> core::memory::unique_pointer<Result>
{
    return { MAKE_NEW(alloc, Type, std::forward<Args>(args)...), core::memory::detail::memsys_deleter<Result>{ alloc } };
}
