#pragma once
#include <ice/mem.hxx>
#include <source_location>
#include <atomic>

namespace ice
{

    template<bool WithDebugInfo>
    struct AllocatorBase;

    using Allocator = ice::AllocatorBase<ice::build::is_debug || ice::build::is_develop>;

    class AllocatorDebugInfo;


    template<bool WithDebugInfo>
    struct AllocatorBase
    {
        static constexpr ice::usize SizeNotTracked = { static_cast<ice::usize::base_type>(0xFFFFEEEE'EEEEFFFF) };
        static constexpr ice::u32 CountNotTracked = 0xFFFF'FFFF;

        static constexpr bool HasDebugInformation = WithDebugInfo;

        AllocatorBase(std::source_location const&) noexcept { }
        AllocatorBase(std::source_location const&, AllocatorBase&) noexcept { }
        AllocatorBase(std::source_location const&, AllocatorBase&, std::u8string_view) noexcept { }

        auto allocate(ice::AllocRequest request) noexcept -> ice::AllocResult
        {
            return do_allocate(request);
        }

        void deallocate(ice::Memory result) noexcept
        {
            return do_deallocate(result);
        }

        template<typename T, typename... Args>
        auto create(Args&&... args) noexcept -> T*
        {
            ice::AllocResult const mem = allocate(ice::size_of<T>);
            return new (mem.result) T{ ice::forward<Args>(args)... };
        }

        template<typename T>
        void destroy(T* object) noexcept
        {
            object->~T();
            return deallocate({ object, ice::size_of<T>, ice::align_of<T> });
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
        virtual void do_deallocate(ice::Memory result) noexcept = 0;
    };

    class AllocatorDebugInfo
    {
    public:
        AllocatorDebugInfo(
            std::source_location src_loc,
            std::u8string_view name
        ) noexcept;

        AllocatorDebugInfo(
            std::source_location const& src_loc,
            std::u8string_view name,
            ice::AllocatorDebugInfo& parent
        ) noexcept;

        auto location() const noexcept -> std::source_location
        {
            return _source_location;
        }

        auto name() const noexcept -> std::u8string_view
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

        auto allocation_size_inuse() const noexcept -> ice::usize
        {
            return { _alloc_inuse.load(std::memory_order_relaxed) };
        }

        void track_child(ice::AllocatorDebugInfo* child_allocator) noexcept;
        auto parent_allocator() const noexcept -> ice::AllocatorDebugInfo const*;
        auto child_allocator() const noexcept -> ice::AllocatorDebugInfo const*;
        auto next_sibling() const noexcept -> ice::AllocatorDebugInfo const*;

    protected:
        void dbg_size_add(ice::usize size) noexcept;
        void dbg_size_sub(ice::usize size) noexcept;

    protected:
        std::source_location const _source_location;
        std::u8string_view const _name;

        ice::AllocatorDebugInfo* const _parent;
        ice::AllocatorDebugInfo* _children = nullptr;
        ice::AllocatorDebugInfo* _next_sibling = nullptr;
        ice::AllocatorDebugInfo* _prev_sibling = nullptr;

        std::atomic<ice::usize::base_type> _alloc_inuse;
        std::atomic<ice::u32> _alloc_count;

        std::atomic<ice::u32> _alloc_total_count;
    };

    template<>
    struct AllocatorBase<true> : AllocatorDebugInfo
    {
        static constexpr ice::usize SizeNotTracked = AllocatorBase<false>::SizeNotTracked;
        static constexpr ice::u32 CountNotTracked = AllocatorBase<false>::CountNotTracked;
        static constexpr bool HasDebugInformation = true;

        AllocatorBase(std::source_location const& src_loc) noexcept;
        AllocatorBase(std::source_location const& src_loc, AllocatorBase& parent) noexcept;
        AllocatorBase(std::source_location const& src_loc, AllocatorBase& parent, std::u8string_view name) noexcept;

        auto allocate(ice::AllocRequest request) noexcept -> ice::AllocResult;
        void deallocate(ice::Memory result) noexcept;

        template<typename T, typename... Args>
        auto create(Args&&... args) noexcept -> T*
        {
            ice::AllocResult const mem = allocate(ice::size_of<T>);
            return new (mem.result) T{ ice::forward<Args>(args)... };
        }

        template<typename T>
        void destroy(T* object) noexcept
        {
            object->~T();
            return deallocate({ object, ice::size_of<T>, ice::align_of<T> });
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
        virtual void do_deallocate(ice::Memory result) noexcept = 0;
    };

} // namespace ice
