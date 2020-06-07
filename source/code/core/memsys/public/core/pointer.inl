
template<class T>
inline core::memory::detail::memsys_default_deleter<T>::memsys_default_deleter(core::allocator& alloc) noexcept
    : _allocator{ &alloc }
{ }

template<class T>
inline core::memory::detail::memsys_default_deleter<T>::memsys_default_deleter(memsys_default_deleter&& other) noexcept
    : _allocator{ other._allocator }
{ }

template<class T>
inline auto core::memory::detail::memsys_default_deleter<T>::operator=(memsys_default_deleter&& other) noexcept -> memsys_default_deleter&
{
    if (this == &other) return *this;
    _allocator = other._allocator;
    other._allocator = nullptr;
    return *this;
}

template<class T>
inline void core::memory::detail::memsys_default_deleter<T>::operator()(T* object) noexcept
{
    MAKE_DELETE((*_allocator), T, object);
}

template<class Result, class Type, class... Args>
inline auto core::memory::make_unique(core::allocator& alloc, Args&&... args) noexcept -> core::memory::unique_pointer<Result>
{
    return { MAKE_NEW(alloc, Type, std::forward<Args>(args)...), core::memory::detail::memsys_default_deleter<Result>{ alloc } };
}


template<class T>
inline core::memory::detail::memsys_custom_deleter<T>::memsys_custom_deleter(
    core::allocator& alloc,
    CustomDeleterFunction* func
) noexcept
    : _allocator{ &alloc }
    , _deleter{ func }
{ }

template<class T>
inline core::memory::detail::memsys_custom_deleter<T>::memsys_custom_deleter(memsys_custom_deleter&& other) noexcept
    : _allocator{ other._allocator }
    , _deleter{ other._deleter }
{ }

template<class T>
inline auto core::memory::detail::memsys_custom_deleter<T>::operator=(memsys_custom_deleter&& other) noexcept -> memsys_custom_deleter&
{
    if (this == &other) return *this;
    _allocator = other._allocator;
    _deleter = other._deleter;
    other._allocator = nullptr;
    other._deleter = nullptr;
    return *this;
}

template<class T>
inline void core::memory::detail::memsys_custom_deleter<T>::operator()(T* object) noexcept
{
    if (_deleter != nullptr)
    {
        _deleter(*_allocator, object);
    }
    else
    {
        MAKE_DELETE((*_allocator), T, object);
    }
}
