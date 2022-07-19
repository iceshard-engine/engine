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

        auto allocate(ice::alloc_request request) noexcept -> ice::alloc_result
        {
            return do_allocate(request);
        }

        void deallocate(ice::alloc_result result) noexcept
        {
            return do_deallocate(result);
        }

        auto allocation_count() const noexcept -> ice::u32
        {
            return CountNotTracked;
        }

        auto total_allocated() const noexcept -> ice::usize
        {
            return SizeNotTracked;
        }

        virtual auto allocated_size(void* ptr) const noexcept -> ice::usize
        {
            return ice::AllocatorBase<false>::SizeNotTracked;
        }

        //! \brief Gives access to debug information for an allocator.
        //!
        //! \note This symbol is not defined for AllocatorBase<false> and accessing it should be done inside a `if constexpr` clause.
        auto debug_info() const noexcept -> ice::AllocatorDebugInfo const&;

    protected:
        virtual ~AllocatorBase() noexcept = default;

        virtual auto do_allocate(ice::alloc_request request) noexcept -> ice::alloc_result = 0;
        virtual void do_deallocate(ice::alloc_result result) noexcept = 0;
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

        auto total_allocated() const noexcept -> ice::usize
        {
            return { _alloc_total_size.load(std::memory_order_relaxed) };
        }

        void track_child(ice::AllocatorDebugInfo* child_allocator) noexcept;
        auto parent_allocator() const noexcept -> ice::AllocatorDebugInfo const*;
        auto child_allocators() const noexcept -> ice::AllocatorDebugInfo const*;
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

        std::atomic<ice::usize::base_type> _alloc_total_size;
        std::atomic<ice::u32> _alloc_count;
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

        auto allocate(ice::alloc_request request) noexcept -> ice::alloc_result;
        void deallocate(ice::alloc_result result) noexcept;

        using AllocatorDebugInfo::allocation_count;
        using AllocatorDebugInfo::total_allocated;

        virtual auto allocated_size(void* ptr) const noexcept -> ice::usize
        {
            return ice::AllocatorBase<false>::SizeNotTracked;
        }

        //! \copy AllocatorBase<false>::debug_info()
        auto debug_info() const noexcept -> ice::AllocatorDebugInfo const&;

    protected:
        virtual ~AllocatorBase() noexcept;

        virtual auto do_allocate(ice::alloc_request request) noexcept -> ice::alloc_result = 0;
        virtual void do_deallocate(ice::alloc_result result) noexcept = 0;
    };

} // namespace ice
