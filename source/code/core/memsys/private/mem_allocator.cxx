/// Copyright 2022 - 2023, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#include <ice/mem_allocator.hxx>
#include <assert.h>
#include <unordered_map>
#include <mutex>

namespace ice
{

    struct AllocInfo
    {
        ice::usize size;
    };

    struct AllocatorDebugInfo::Internal
    {
        std::mutex mtx;
        std::unordered_map<void*, AllocInfo> allocs;
        std::atomic<ice::u64> _allocated_inuse;

        void insert(ice::AllocResult const& result) noexcept
        {
            std::lock_guard<std::mutex> lk{ mtx };
            allocs.emplace(result.memory, AllocInfo{ result.size });

            _allocated_inuse.fetch_add(result.size.value, std::memory_order_relaxed);
        }

        void remove(void* pointer) noexcept
        {
            std::lock_guard<std::mutex> lk{ mtx };
            _allocated_inuse.fetch_sub(allocs.at(pointer).size.value, std::memory_order_relaxed);
            allocs.erase(pointer);
        }
    };

    AllocatorDebugInfo::AllocatorDebugInfo(
        std::source_location src_loc,
        std::string_view name
    ) noexcept
        : _source_location{ src_loc }
        , _name{ name }
        , _parent{ nullptr }
        , _children{ nullptr }
        , _next_sibling{ nullptr }
        , _prev_sibling{ nullptr }
        , _alloc_count{ 0 }
        , _alloc_total_count{ 0 }
        , _internal{ new Internal{} }
    {
    }

    AllocatorDebugInfo::AllocatorDebugInfo(
        std::source_location const& src_loc,
        std::string_view name,
        ice::AllocatorDebugInfo& parent
    ) noexcept
        : _source_location{ src_loc }
        , _name{ name }
        , _parent{ &parent }
        , _children{ nullptr }
        , _next_sibling{ nullptr }
        , _prev_sibling{ nullptr }
        , _alloc_count{ 0 }
        , _alloc_total_count{ 0 }
        , _internal{ new Internal{} }
    {
        _parent->track_child(this);
    }

    AllocatorDebugInfo::~AllocatorDebugInfo() noexcept
    {
        if (_parent)
        {
            _parent->remove_child(this);
        }

        delete _internal;
    }

    auto AllocatorDebugInfo::allocation_size_inuse() const noexcept -> ice::usize
    {
        return { _internal->_allocated_inuse };
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

    void AllocatorDebugInfo::remove_child(ice::AllocatorDebugInfo* child_allocator) noexcept
    {
        if (child_allocator->_prev_sibling == nullptr)
        {
            _children = child_allocator->_next_sibling;
            if (_children)
            {
                _children->_prev_sibling = nullptr;
            }
        }
        else
        {
            child_allocator->_prev_sibling->_next_sibling = child_allocator->_next_sibling;
        }
    }

    auto AllocatorDebugInfo::parent_allocator() const noexcept -> ice::AllocatorDebugInfo const*
    {
        return _parent;
    }

    auto AllocatorDebugInfo::child_allocator() const noexcept -> ice::AllocatorDebugInfo const*
    {
        return _children;
    }

    auto AllocatorDebugInfo::next_sibling() const noexcept -> ice::AllocatorDebugInfo const*
    {
        return _next_sibling;
    }

    void AllocatorDebugInfo::dbg_count_add() noexcept
    {
        _alloc_count.fetch_add(1, std::memory_order_relaxed);
        _alloc_total_count.fetch_add(1, std::memory_order_relaxed);
    }

    void AllocatorDebugInfo::dbg_count_sub() noexcept
    {
        _alloc_count.fetch_sub(1, std::memory_order_relaxed);
    }

    AllocatorBase<true>::AllocatorBase(std::source_location const& src_loc) noexcept
        : AllocatorDebugInfo{ src_loc, src_loc.function_name() }
    {
    }

    AllocatorBase<true>::AllocatorBase(std::source_location const& src_loc, AllocatorBase& parent) noexcept
        : AllocatorDebugInfo{ src_loc, src_loc.function_name(), parent }
    {
    }

    AllocatorBase<true>::AllocatorBase(std::source_location const& src_loc, AllocatorBase& parent, std::string_view name) noexcept
        : AllocatorDebugInfo{ src_loc, name, parent }
    {
    }

    auto AllocatorBase<true>::allocate(ice::AllocRequest request) noexcept -> ice::AllocResult
    {
        ICE_ASSERT_CORE(request.size != 0_B);
        ice::AllocResult result = do_allocate(request);

        _internal->insert(result);
        dbg_count_add();
        return result;
    }

    void AllocatorBase<true>::deallocate(void* pointer) noexcept
    {
        if (pointer == nullptr) return;

        _internal->remove(pointer);
        dbg_count_sub();
        do_deallocate(pointer);
    }

    auto AllocatorBase<true>::debug_info() const noexcept -> ice::AllocatorDebugInfo const&
    {
        return *this;
    }

    AllocatorBase<true>::~AllocatorBase() noexcept
    {
        ICE_ASSERT_CORE(allocation_count() == 0);
    }

} // namespace ice
