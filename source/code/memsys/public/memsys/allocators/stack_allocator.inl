
template<uint32_t BUFFER_SIZE>
inline stack_allocator<BUFFER_SIZE>::stack_allocator() noexcept
    : _static_memory{ }
    , _next_free{ _static_memory }
{
}

template<uint32_t BUFFER_SIZE>
inline void* stack_allocator<BUFFER_SIZE>::allocate(uint32_t size, uint32_t align /*= DEFAULT_ALIGN*/) noexcept
{
    void* free_location = utils::align_forward(_next_free, align);
    _next_free = utils::pointer_add(free_location, size);

    IS_ASSERT(utils::pointer_distance(_static_memory, _next_free) <= BUFFER_SIZE, "Stack allocator overgrown by {} bytes!", utils::pointer_distance(_static_memory, _next_free));
    return free_location;
}

template<uint32_t BUFFER_SIZE>
inline void stack_allocator<BUFFER_SIZE>::deallocate(void*) noexcept
{
    /* not implemented */
}

template<uint32_t BUFFER_SIZE>
inline uint32_t stack_allocator<BUFFER_SIZE>::allocated_size(void*) noexcept
{
    return SIZE_NOT_TRACKED;
}

template<uint32_t BUFFER_SIZE>
inline uint32_t stack_allocator<BUFFER_SIZE>::total_allocated() noexcept
{
    return SIZE_NOT_TRACKED;
}

template<uint32_t BUFFER_SIZE>
inline void stack_allocator<BUFFER_SIZE>::clear() noexcept
{
    _next_free = _static_memory;
}
