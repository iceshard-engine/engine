
//! Allocates and constructs the given object using the provided arguments.
template<class T, class... Args>
auto core::allocator::make(Args&&... args) noexcept -> T*
{
    return new (allocate(sizeof(T), alignof(T))) T(std::forward<Args>(args)...);
}

/// Destroys the given object and deallocates the memory used
template<class T>
void core::allocator::destroy(T* ptr) noexcept
{
    if (ptr)
    {
        ptr->~T();
        deallocate(ptr);
    }
}

/// Destroys the given object and deallocates the memory used
template<class T>
void core::allocator::destroy(void* ptr) noexcept
{
    if (ptr)
    {
        reinterpret_cast<T*>(ptr)->~T();
        deallocate(ptr);
    }
}
