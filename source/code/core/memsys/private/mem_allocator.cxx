#include <ice/mem_allocator.hxx>
#include <assert.h>

namespace ice
{

    AllocatorDebugInfo::AllocatorDebugInfo(
        std::source_location src_loc,
        std::u8string_view name
    ) noexcept
        : _source_location{ src_loc }
        , _name{ name }
        , _parent{ nullptr }
    {
    }

    AllocatorDebugInfo::AllocatorDebugInfo(
        std::source_location const& src_loc,
        std::u8string_view name,
        ice::AllocatorDebugInfo& parent
    ) noexcept
        : _source_location{ src_loc }
        , _name{ name }
        , _parent{ &parent }
    {
        _parent->track_child(this);
    }

    void AllocatorDebugInfo::track_child(ice::AllocatorDebugInfo* child_allocator) noexcept
    {
        child_allocator->_next_sibling = _children;
        if (_children != nullptr)
        {
            _children->_prev_sibling = child_allocator;
        }
        _children = child_allocator;
        _children->_prev_sibling = nullptr;
    }

    auto AllocatorDebugInfo::parent_allocator() const noexcept -> ice::AllocatorDebugInfo const*
    {
        return _parent;
    }

    auto AllocatorDebugInfo::child_allocators() const noexcept -> ice::AllocatorDebugInfo const*
    {
        return _children;
    }

    auto AllocatorDebugInfo::next_sibling() const noexcept -> ice::AllocatorDebugInfo const*
    {
        return _next_sibling;
    }

    void AllocatorDebugInfo::dbg_size_add(ice::usize size) noexcept
    {
        _alloc_total_size.fetch_add(size.value, std::memory_order_relaxed);
        _alloc_count.fetch_add(size != 0_B, std::memory_order_relaxed);
    }

    void AllocatorDebugInfo::dbg_size_sub(ice::usize size) noexcept
    {
        _alloc_total_size.fetch_sub(size.value, std::memory_order_relaxed);
        _alloc_count.fetch_sub(size != 0_B, std::memory_order_relaxed);
    }

    AllocatorBase<true>::AllocatorBase(std::source_location const& src_loc) noexcept
        : AllocatorDebugInfo{ src_loc, reinterpret_cast<ice::utf8 const*>(src_loc.function_name()) }
    {
    }

    AllocatorBase<true>::AllocatorBase(std::source_location const& src_loc, AllocatorBase& parent) noexcept
        : AllocatorDebugInfo{ src_loc, reinterpret_cast<ice::utf8 const*>(src_loc.function_name()), parent }
    {
    }

    AllocatorBase<true>::AllocatorBase(std::source_location const& src_loc, AllocatorBase& parent, std::u8string_view name) noexcept
        : AllocatorDebugInfo{ src_loc, name, parent }
    {
    }

    auto AllocatorBase<true>::allocate(ice::alloc_request request) noexcept -> ice::alloc_result
    {
        ice::alloc_result result = do_allocate(request);
        dbg_size_add(result.size);
        return result;
    }

    void AllocatorBase<true>::deallocate(ice::alloc_result result) noexcept
    {
        dbg_size_sub(result.size);
        do_deallocate(result);
    }

    auto AllocatorBase<true>::debug_info() const noexcept -> ice::AllocatorDebugInfo const&
    {
        return *this;
    }

    AllocatorBase<true>::~AllocatorBase() noexcept
    {
        assert(total_allocated() == 0_B);
        assert(allocation_count() == 0);
    }

} // namespace ice
