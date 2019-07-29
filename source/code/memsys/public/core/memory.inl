
inline auto core::memory::utils::align_forward(void* p, uint32_t align) noexcept -> void*
{
    uintptr_t pi = uintptr_t(p);
    const uint32_t mod = pi % align;
    if (mod)
        pi += (align - mod);
    return reinterpret_cast<void*>(pi);
}

inline auto core::memory::utils::align_forward(const void* p, uint32_t align) noexcept -> const void*
{
    uintptr_t pi = uintptr_t(p);
    const uint32_t mod = pi % align;
    if (mod)
        pi += (align - mod);
    return reinterpret_cast<void*>(pi);
}

inline auto core::memory::utils::pointer_add(void* ptr, uint32_t bytes) noexcept -> void*
{
    return reinterpret_cast<void*>(reinterpret_cast<char*>(ptr) + bytes);
}

inline auto core::memory::utils::pointer_add(const void* ptr, uint32_t bytes) noexcept -> const void*
{
    return reinterpret_cast<const void*>(reinterpret_cast<const char*>(ptr) + bytes);
}

inline auto core::memory::utils::pointer_sub(void* ptr, uint32_t bytes) noexcept -> void*
{
    return reinterpret_cast<void*>(reinterpret_cast<char*>(ptr) - bytes);
}

inline auto core::memory::utils::pointer_sub(const void* ptr, uint32_t bytes) noexcept -> const void*
{
    return reinterpret_cast<const void*>(reinterpret_cast<const char*>(ptr) - bytes);
}

inline auto core::memory::utils::pointer_distance(void* from, void* to) noexcept -> int32_t
{
    auto distance = reinterpret_cast<char*>(to) - reinterpret_cast<char*>(from);
    return static_cast<int32_t>(distance);
}

inline auto core::memory::utils::pointer_distance(const void* from, const void* to) noexcept -> int32_t
{
    return static_cast<int32_t>(reinterpret_cast<const char*>(to) - reinterpret_cast<const char*>(from));
}
