#pragma once
#include <ice/module.hxx>
#include <ice/mem_allocator_host.hxx>
#include <ice/mem_allocator_proxy.hxx>

namespace ice::detail
{

    struct DebugAllocatorAPI
    {
        static constexpr ice::StringID Constant_APIName = "iceshard.task.debug-api"_sid;
        static constexpr ice::u32 Constant_APIVersion = 1;

        ice::Allocator* allocator_ptr;
        ice::String allocator_pool;
    };

#if !ICE_RELEASE
    struct TaskDebugAllocator final : public ice::Module<TaskDebugAllocator>
    {
        static auto pool() noexcept -> ice::String
        {
            return _allocator_pool;
        }

        static auto allocator() noexcept -> ice::Allocator&
        {
            static HostAllocator fallback_alloc;
            return _allocator_ptr ? *_allocator_ptr : fallback_alloc;
        }

        static auto allocate(size_t size) noexcept -> void*
        {
            return allocator().allocate(ice::usize{ size }).memory;
        }

        static void deallocate(void* pointer) noexcept
        {
            allocator().deallocate(pointer);
        }

        static bool on_load(ice::Allocator& alloc, ice::ModuleNegotiator auto const& negotiator) noexcept
        {
            DebugAllocatorAPI api;
            if (negotiator.query_api(api))
            {
                _allocator_ptr = api.allocator_ptr;
                _allocator_pool = api.allocator_pool;
            }
            return true;
        }

        static void on_unload(ice::Allocator& alloc) noexcept
        {
            _allocator_ptr = nullptr;
        }

        IS_WORKAROUND_MODULE_INITIALIZATION(TaskDebugAllocator);

    private:
        static inline ice::Allocator* _allocator_ptr{ };
        static inline ice::String _allocator_pool;
    };
#endif

} // namespace ice
