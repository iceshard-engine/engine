/// Copyright 2022 - 2024, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/mem.hxx>
#include <source_location>
#include <atomic>

namespace ice
{


    template<bool WithDebugInfo>
    struct AllocatorBase
    {
        static constexpr ice::usize SizeNotTracked = { static_cast<ice::usize::base_type>(0xFFFFEEEE'EEEEFFFF) };
        static constexpr ice::u32 CountNotTracked = 0xFFFF'FFFF;

        static constexpr bool HasDebugInformation = WithDebugInfo;

        AllocatorBase(std::source_location const&) noexcept { }
        AllocatorBase(std::source_location const&, std::string_view) noexcept { }
        AllocatorBase(std::source_location const&, AllocatorBase&) noexcept { }
        AllocatorBase(std::source_location const&, AllocatorBase&, std::string_view) noexcept { }

        auto allocate(ice::AllocRequest request) noexcept -> ice::AllocResult
        {
            return do_allocate(request);
        }

        template<typename T> requires std::is_trivial_v<T>
        auto allocate(ice::u64 count = 1) noexcept -> T*
        {
            return reinterpret_cast<T*>(allocate(AllocRequest{ ice::meminfo_of<T> *count }).memory);
        }

        void deallocate(void* pointer) noexcept
        {
            if (pointer == nullptr) return;
            return do_deallocate(pointer);
        }

        void deallocate(ice::Memory result) noexcept
        {
            return deallocate(result.location);
        }

        template<typename T, typename... Args>
        auto create(Args&&... args) noexcept -> T*
        {
            ice::AllocResult const mem = allocate(ice::meminfo_of<T>);
            return new (mem.memory) T{ ice::forward<Args>(args)... };
        }

        template<typename T>
        void destroy(T* object) noexcept
        {
            object->~T();
            return deallocate(object);
        }

        auto allocation_count() const noexcept -> ice::u32
        {
            return CountNotTracked;
        }

        auto allocation_total_count() const noexcept -> ice::u32
        {
            return CountNotTracked;
        }

        auto allocation_size_inuse() const noexcept -> ice::usize
        {
            return SizeNotTracked;
        }

        auto allocation_size_watermark() const noexcept -> ice::usize
        {
            return SizeNotTracked;
        }

        virtual auto allocation_size(void* ptr) const noexcept -> ice::usize
        {
            return SizeNotTracked;
        }

        //! \brief Gives access to debug information for an allocator.
        //!
        //! \note This symbol is not defined for AllocatorBase<false> and accessing it should be done inside a `if constexpr` clause.
        auto debug_info() const noexcept -> ice::AllocatorDebugInfo const&;

    protected:
        virtual ~AllocatorBase() noexcept = default;

        virtual auto do_allocate(ice::AllocRequest request) noexcept -> ice::AllocResult = 0;
        virtual void do_deallocate(void* pointer) noexcept = 0;
    };

    class AllocatorDebugInfo
    {
    public:
        AllocatorDebugInfo(
            std::source_location src_loc,
            std::string_view name
        ) noexcept;

        AllocatorDebugInfo(
            std::source_location const& src_loc,
            std::string_view name,
            ice::AllocatorDebugInfo& parent
        ) noexcept;

        virtual ~AllocatorDebugInfo() noexcept;

        auto location() const noexcept -> std::source_location
        {
            return _source_location;
        }

        auto name() const noexcept -> std::string_view
        {
            return _name;
        }

        auto allocation_count() const noexcept -> ice::u32
        {
            return _alloc_count.load(std::memory_order_relaxed);
        }

        auto allocation_total_count() const noexcept -> ice::u32
        {
            return _alloc_total_count.load(std::memory_order_relaxed);
        }

        auto allocation_size_inuse() const noexcept -> ice::usize;

        auto allocation_size_watermark() const noexcept -> ice::usize;

        void track_child(ice::AllocatorDebugInfo* child_allocator) noexcept;
        void remove_child(ice::AllocatorDebugInfo* child_allocator) noexcept;

        auto parent_allocator() const noexcept -> ice::AllocatorDebugInfo const*;
        auto child_allocator() const noexcept -> ice::AllocatorDebugInfo const*;
        auto next_sibling() const noexcept -> ice::AllocatorDebugInfo const*;

    protected:
        void dbg_count_add() noexcept;
        void dbg_count_sub() noexcept;

    protected:
        std::source_location const _source_location;
        std::string_view const _name;

        ice::AllocatorDebugInfo* const _parent;
        ice::AllocatorDebugInfo* _children;
        ice::AllocatorDebugInfo* _next_sibling;
        ice::AllocatorDebugInfo* _prev_sibling;

        std::atomic<ice::u32> _alloc_count;
        std::atomic<ice::u32> _alloc_total_count;

        struct Internal;
        Internal* _internal;
    };

    template<>
    struct AllocatorBase<true> : AllocatorDebugInfo
    {
        static constexpr ice::usize SizeNotTracked = AllocatorBase<false>::SizeNotTracked;
        static constexpr ice::u32 CountNotTracked = AllocatorBase<false>::CountNotTracked;
        static constexpr bool HasDebugInformation = true;

        AllocatorBase(std::source_location const& src_loc) noexcept;
        AllocatorBase(std::source_location const& src_loc, std::string_view name) noexcept;
        AllocatorBase(std::source_location const& src_loc, AllocatorBase& parent) noexcept;
        AllocatorBase(std::source_location const& src_loc, AllocatorBase& parent, std::string_view name) noexcept;

        auto allocate(ice::AllocRequest request) noexcept -> ice::AllocResult;
        template<typename T> requires std::is_trivial_v<T>
        auto allocate(ice::u64 count = 1) noexcept -> T*
        {
            return reinterpret_cast<T*>(allocate(AllocRequest{ ice::meminfo_of<T> * count }).memory);
        }

        void deallocate(void* pointer) noexcept;
        void deallocate(ice::Memory memory) noexcept
        {
            if (memory.location == nullptr) return;
            return deallocate(memory.location);
        }

        template<typename T, typename... Args>
        auto create(Args&&... args) noexcept -> T*
        {
            ice::AllocResult const mem = allocate(ice::meminfo_of<T>);
            return new (mem.memory) T{ ice::forward<Args>(args)... };
        }

        template<typename T> requires (std::is_const_v<T> == false)
        void destroy(T* object) noexcept
        {
            object->~T();
            return deallocate(object);
        }

        using AllocatorDebugInfo::allocation_count;
        using AllocatorDebugInfo::allocation_total_count;
        using AllocatorDebugInfo::allocation_size_inuse;

        virtual auto allocation_size(void* ptr) const noexcept -> ice::usize
        {
            return ice::AllocatorBase<false>::SizeNotTracked;
        }

        //! \copy AllocatorBase<false>::debug_info()
        auto debug_info() const noexcept -> ice::AllocatorDebugInfo const&;

    protected:
        virtual ~AllocatorBase() noexcept;

        virtual auto do_allocate(ice::AllocRequest request) noexcept -> ice::AllocResult = 0;
        virtual void do_deallocate(void* pointer) noexcept = 0;
    };

} // namespace ice
