#pragma once
#include <ice/mem_allocator.hxx>

namespace ice
{

    template<typename T>
    struct UniquePtr;

    template<typename T, typename... Args>
    inline auto make_unique(
        ice::Allocator& alloc,
        Args&&... args
    ) noexcept -> ice::UniquePtr<T>;


    template<typename T>
    using UniquePtrCustomDeleter = void(ice::Allocator*, T*) noexcept;

    template<typename T, typename... Args>
    inline auto make_unique(
        ice::Allocator& alloc,
        ice::UniquePtrCustomDeleter<T>* fn_deleter,
        Args&&... args
    ) noexcept -> ice::UniquePtr<T>;


    template<typename T>
    struct UniquePtr
    {
        struct UserDeleterInfo;

        inline UniquePtr() noexcept;
        inline ~UniquePtr() noexcept;

        inline explicit UniquePtr(ice::Allocator* alloc, T* ptr) noexcept;
        inline explicit UniquePtr(UserDeleterInfo* info, T* ptr) noexcept;

        inline UniquePtr(UniquePtr&& other) noexcept;
        template<typename U> requires std::is_base_of_v<T, U>
        inline UniquePtr(UniquePtr<U>&& other) noexcept;

        inline auto operator=(UniquePtr&& other) noexcept -> UniquePtr&;
        template<typename U> requires std::is_base_of_v<T, U>
        inline auto operator=(UniquePtr<U>&& other) noexcept -> UniquePtr&;

        bool operator==(std::nullptr_t) const noexcept { return _ptr == nullptr; }

        auto operator->() noexcept -> T* { return _ptr; }
        auto operator->() const noexcept -> T const* { return _ptr; }

        auto operator*() noexcept -> T& { return *_ptr; }
        auto operator*() const noexcept -> T const& { return *_ptr; }

        inline auto get() noexcept -> T* { return _ptr; }
        inline void reset() noexcept;

        Allocator* _alloc;
        T* _ptr;
    };

    template<typename T>
    struct UniquePtr<T>::UserDeleterInfo
    {
        ice::Allocator* alloc;
        ice::UniquePtrCustomDeleter<T>* fn_deleter;
    };

    template<typename T>
    inline UniquePtr<T>::UniquePtr() noexcept
        : _alloc{ nullptr }
        , _ptr{ nullptr }
    { }

    template<typename T>
    inline UniquePtr<T>::~UniquePtr() noexcept
    {
        reset();
    }

    template<typename T>
    inline UniquePtr<T>::UniquePtr(ice::Allocator* alloc, T* ptr) noexcept
        : _alloc{ alloc }
        , _ptr{ ptr }
    { }

    template<typename T>
    inline UniquePtr<T>::UniquePtr(UserDeleterInfo* info, T* ptr) noexcept
        : _alloc{ reinterpret_cast<ice::Allocator*>(reinterpret_cast<ice::uptr>(info) | 0x1) }
        , _ptr{ ptr }
    { }

    template<typename T>
    inline UniquePtr<T>::UniquePtr(UniquePtr&& other) noexcept
        : _alloc{ std::exchange(other._alloc, nullptr) }
        , _ptr{ std::exchange(other._ptr, nullptr) }
    {
    }

    template<typename T>
    template<typename U> requires std::is_base_of_v<T, U>
    inline UniquePtr<T>::UniquePtr(UniquePtr<U>&& other) noexcept
        : _alloc{ std::exchange(other._alloc, nullptr) }
        , _ptr{ std::exchange(other._ptr, nullptr) }
    {
    }

    template<typename T>
    inline auto UniquePtr<T>::operator=(UniquePtr&& other) noexcept -> UniquePtr&
    {
        if (this != &other)
        {
            reset();

            _alloc = std::exchange(other._alloc, nullptr);
            _ptr = std::exchange(other._ptr, nullptr);
        }
        return *this;
    }

    template<typename T>
    template<typename U> requires std::is_base_of_v<T, U>
    inline auto UniquePtr<T>::operator=(UniquePtr<U>&& other) noexcept -> UniquePtr&
    {
        reset();

        _alloc = std::exchange(other._alloc, nullptr);
        _ptr = std::exchange(other._ptr, nullptr);
        return *this;
    }

    template<typename T>
    inline void UniquePtr<T>::reset() noexcept
    {
        if (_ptr == nullptr)
        {
            return;
        }

        // If we don't have a 'special' case
        if ((reinterpret_cast<ice::uptr>(_alloc) & 0x1) == 0x0)
        {
            _alloc->destroy(_ptr);
        }
        else
        {
            UserDeleterInfo const* deleter_info = reinterpret_cast<UserDeleterInfo const*>(
                reinterpret_cast<ice::uptr>(_alloc) & ~0x1
            );
            deleter_info->fn_deleter(deleter_info->alloc, _ptr);
        }

        _ptr = nullptr;
    }

    template<typename T, typename... Args>
    inline auto make_unique(
        ice::Allocator& alloc,
        Args&&... args
    ) noexcept -> ice::UniquePtr<T>
    {
        ice::AllocResult const mem = alloc.allocate(ice::meminfo_of<T>);

        T* const object = new (mem.memory) T{ ice::forward<Args>(args)... };
        return ice::UniquePtr<T>{ &alloc, object };
    }

    template<typename T, typename... Args>
    inline auto make_unique(
        ice::Allocator& alloc,
        ice::UniquePtrCustomDeleter<T>* fn_deleter,
        Args&&... args
    ) noexcept -> ice::UniquePtr<T>
    {
        using DeleterInfo = typename ice::UniquePtr<T>::UserDeleterInfo;

        ice::meminfo total_memory = ice::meminfo_of<T>;
        ice::usize const udi_offset = total_memory += ice::meminfo_of<DeleterInfo>;

        ice::AllocResult const mem = alloc.allocate(total_memory);

        T* const object = new (mem.result) T{ std::forward<Args>(args)... };
        DeleterInfo* const deleter_info = new (ice::ptr_add(mem.result, udi_offset)) DeleterInfo{ &alloc, fn_deleter };

        return ice::UniquePtr<T>{ deleter_info, object };
    }

} // namespace ice
