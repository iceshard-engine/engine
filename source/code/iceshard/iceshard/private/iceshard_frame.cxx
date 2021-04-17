#include "iceshard_frame.hxx"
#include <ice/engine_request.hxx>
#include <ice/pod/hash.hxx>
#include <ice/assert.hxx>

namespace ice
{

    namespace detail
    {

        static constexpr ice::u32 KiB = 1024;
        static constexpr ice::u32 MiB = 1024 * KiB;

        static constexpr ice::u32 InputsAllocatorCapacity = 1 * MiB;
        static constexpr ice::u32 RequestAllocatorCapacity = 1 * MiB;
        static constexpr ice::u32 MessageAllocatorCapacity = 20 * MiB;
        static constexpr ice::u32 StorageAllocatorCapacity = 1 * MiB;
        static constexpr ice::u32 DataAllocatorCapacity = 206 * MiB;

        static ice::u32 global_frame_counter = 0;

    } // namespace detail

    IceshardMemoryFrame::IceshardMemoryFrame(
        ice::memory::ScratchAllocator& alloc
    ) noexcept
        : ice::EngineFrame{ }
        , _allocator{ alloc }
        , _inputs_allocator{ _allocator, detail::InputsAllocatorCapacity }
        , _request_allocator{ _allocator, detail::RequestAllocatorCapacity }
        , _message_allocator{ _allocator, detail::MessageAllocatorCapacity }
        , _storage_allocator{ _allocator, detail::StorageAllocatorCapacity }
        , _data_allocator{ _allocator, detail::DataAllocatorCapacity }
        , _input_events{ _allocator }
        , _requests{ _request_allocator }
        , _named_objects{ _storage_allocator }
    {
        detail::global_frame_counter += 1;

        ice::pod::array::reserve(_input_events, (detail::InputsAllocatorCapacity / sizeof(ice::input::InputEvent)) - 10);
        ice::pod::array::reserve(_requests, (detail::RequestAllocatorCapacity / sizeof(ice::EngineRequest)) - 10);
        ice::pod::hash::reserve(_named_objects, (detail::StorageAllocatorCapacity / (sizeof(ice::pod::Hash<ice::uptr>::Entry) + sizeof(ice::u32))) - 10);
    }

    IceshardMemoryFrame::~IceshardMemoryFrame() noexcept
    {
        for (auto const& entry : _named_objects)
        {
            _data_allocator.deallocate(entry.value);
        }
    }

    auto IceshardMemoryFrame::memory_consumption() noexcept -> ice::u32
    {
        return sizeof(IceshardMemoryFrame)
            + _inputs_allocator.total_allocated()
            + _request_allocator.total_allocated()
            + _message_allocator.total_allocated()
            + _storage_allocator.total_allocated()
            + _data_allocator.total_allocated();
    }

    auto IceshardMemoryFrame::input_events() noexcept -> ice::pod::Array<ice::input::InputEvent>&
    {
        return _input_events;
    }

    auto IceshardMemoryFrame::input_events() const noexcept -> ice::Span<ice::input::InputEvent const>
    {
        return _input_events;
    }

    void IceshardMemoryFrame::push_requests(
        ice::Span<EngineRequest const> requests
    ) noexcept
    {
        ice::pod::array::push_back(_requests, requests);
    }

    auto IceshardMemoryFrame::named_data(
        ice::StringID_Arg name
    ) noexcept -> void*
    {
        return ice::pod::hash::get(_named_objects, ice::hash(name), nullptr);
    }

    auto IceshardMemoryFrame::named_data(
        ice::StringID_Arg name
    ) const noexcept -> void const*
    {
        return ice::pod::hash::get(_named_objects, ice::hash(name), nullptr);
    }

    auto IceshardMemoryFrame::allocate_named_data(
        ice::StringID_Arg name,
        ice::u32 size,
        ice::u32 alignment
    ) noexcept -> void*
    {
        ice::u64 const name_hash = ice::hash(name);
        ICE_ASSERT(
            ice::pod::hash::has(_named_objects, name_hash) == false,
            "An object with this name `{}` already exists in this frame!",
            ice::stringid_hint(name)
        );

        void* object_ptr = _data_allocator.allocate(size, alignment);
        ice::pod::hash::set(_named_objects, name_hash, object_ptr);
        return object_ptr;
    }

    auto IceshardMemoryFrame::requests() const noexcept -> ice::Span<EngineRequest const>
    {
        return _requests;
    }

} // namespace ice
