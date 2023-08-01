/// Copyright 2022 - 2023, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

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
    using UniquePtrCustomDeleter = void(T*) noexcept;

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
        inline explicit UniquePtr(ice::UniquePtrCustomDeleter<T>* deleter_fn, T* ptr) noexcept;

        inline UniquePtr(UniquePtr&& other) noexcept;
        template<typename U> requires std::is_base_of_v<T, U>
        inline UniquePtr(UniquePtr<U>&& other) noexcept;

        inline auto operator=(UniquePtr&& other) noexcept -> UniquePtr&;
        template<typename U> requires std::is_base_of_v<T, U>
        inline auto operator=(UniquePtr<U>&& other) noexcept -> UniquePtr&;
        inline auto operator=(std::nullptr_t) noexcept -> UniquePtr&;

        bool operator==(std::nullptr_t) const noexcept { return _ptr == nullptr; }

        auto operator->() const noexcept -> T* { return _ptr; }
        auto operator*() const noexcept -> T& { return *_ptr; }

        inline auto get() const noexcept -> T* { return _ptr; }
        inline void reset() noexcept;

        Allocator* _alloc;
        T* _ptr;
        void* _deleter;
    };

    template<typename T>
    struct UniquePtr<T>::UserDeleterInfo
    {
        //ice::Allocator* alloc;
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
        , _deleter{ nullptr }
    { }

    template<typename T>
    inline UniquePtr<T>::UniquePtr(ice::UniquePtrCustomDeleter<T>* deleter_fn, T* ptr) noexcept
        : _alloc{ nullptr }
        , _ptr{ ptr }
        , _deleter{ reinterpret_cast<void*>(deleter_fn) }
    { }

    template<typename T>
    inline UniquePtr<T>::UniquePtr(UniquePtr&& other) noexcept
        : _alloc{ std::exchange(other._alloc, nullptr) }
        , _ptr{ std::exchange(other._ptr, nullptr) }
        , _deleter{ std::exchange(other._deleter, nullptr) }
    {
    }

    template<typename T>
    template<typename U> requires std::is_base_of_v<T, U>
    inline UniquePtr<T>::UniquePtr(UniquePtr<U>&& other) noexcept
        : _alloc{ std::exchange(other._alloc, nullptr) }
        , _ptr{ std::exchange(other._ptr, nullptr) }
        , _deleter{ std::exchange(other._deleter, nullptr) }
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
            _deleter = std::exchange(other._deleter, nullptr);
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
        _deleter = std::exchange(other._deleter, nullptr);
        return *this;
    }

    template<typename T>
    inline auto UniquePtr<T>::operator=(std::nullptr_t) noexcept -> UniquePtr&
    {
        reset();
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
        if (_alloc != nullptr)
        {
            if constexpr (ice::is_type_complete<T>)
            {
                ICE_ASSERT_CORE(_deleter == nullptr); // TODO: Implement various deleters
                _alloc->destroy(_ptr);
            }
            else
            {
                ICE_ASSERT_CORE(_deleter != nullptr); // MEMORY LEAK INBOUND! Cannot delete object of incomplete type!
            }
        }
        else
        {
            ICE_ASSERT_CORE(_deleter != nullptr); // TODO: Implement various deleters
            ice::UniquePtrCustomDeleter<T>* fn_deleter = std::bit_cast<ice::UniquePtrCustomDeleter<T>*>(_deleter);
            fn_deleter(_ptr);
        }

        _alloc = nullptr;
        _ptr = nullptr;
        _deleter = nullptr;
    }

    template<typename T, typename... Args>
    inline auto make_unique(
        ice::Allocator& alloc,
        Args&&... args
    ) noexcept -> ice::UniquePtr<T>
    {
        return ice::UniquePtr<T>{ &alloc, alloc.create<T>(ice::forward<Args>(args)...) };
    }

    //template<typename T, typename... Args>
    //inline auto make_unique(
    //    ice::Allocator& alloc,
    //    ice::UniquePtrCustomDeleter<T>* fn_deleter,
    //    Args&&... args
    //) noexcept -> ice::UniquePtr<T>
    //{
    //    using DeleterInfo = typename ice::UniquePtr<T>::UserDeleterInfo;

    //    ice::meminfo total_memory = ice::meminfo_of<T>;
    //    ice::usize const udi_offset = total_memory += ice::meminfo_of<DeleterInfo>;

    //    ice::AllocResult const mem = alloc.allocate(total_memory);

    //    T* const object = new (mem.memory) T{ std::forward<Args>(args)... };
    //    DeleterInfo* const deleter_info = new (ice::ptr_add(mem.memory, udi_offset)) DeleterInfo{ &alloc, fn_deleter };

    //    return ice::UniquePtr<T>{ deleter_info, object };
    //}

    template<typename T, typename... Args>
    inline auto make_unique(
        ice::UniquePtrCustomDeleter<T>* fn_deleter,
        T* instanced_object
    ) noexcept -> ice::UniquePtr<T>
    {
        return ice::UniquePtr<T>{ fn_deleter, instanced_object };
    }

} // namespace ice
