
template<class T>
inline detail::mooned_deleter<T>::mooned_deleter(memsys::allocator& alloc) noexcept
    : _allocator{ &alloc }
{ }

template<class T>
inline detail::mooned_deleter<T>::mooned_deleter(mooned_deleter&& other) noexcept
    : _allocator{ other._allocator }
{ }

template<class T>
inline auto detail::mooned_deleter<T>::operator=(mooned_deleter&& other) noexcept -> mooned_deleter&
{
    if (this == &other) return *this;
    _allocator = other._allocator;
    other._allocator = nullptr;
    return *this;
}

template<class T>
inline void detail::mooned_deleter<T>::operator()(T* object) noexcept
{
    MAKE_DELETE((*_allocator), T, object);
}

template<class T, class... Args>
inline auto make_unique(memsys::allocator& alloc, Args&&... args) noexcept -> unique_pointer<T>
{
    return { MAKE_NEW(alloc, T, std::forward<Args>(args)...), detail::mooned_deleter<T>{ alloc } };
}
