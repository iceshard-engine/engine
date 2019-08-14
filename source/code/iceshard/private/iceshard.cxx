#include <iceshard/engine.hxx>

#include <resource/system.hxx>
#include <input_system/system.hxx>
#include <input_system/module.hxx>

#include <core/allocators/proxy_allocator.hxx>
#include <core/allocators/scratch_allocator.hxx>

#include <cppcoro/task.hpp>
#include <cppcoro/sync_wait.hpp>
#include <cppcoro/when_all_ready.hpp>

#include "frame.hxx"

namespace iceshard
{
    namespace detail
    {

        static constexpr auto MiB = 1024u * 1024u;

        static constexpr auto FrameAllocatorCapacity = 256u * detail::MiB;

        template<typename T, uint32_t Size>
        constexpr auto array_element_count(T(&)[Size]) noexcept
        {
            return Size;
        }

    } // namespace detail


    class IceShardEngine final : public iceshard::Engine
    {
    public:
        IceShardEngine(core::allocator& alloc, resource::ResourceSystem& resources) noexcept
            : _allocator{ "iceshard-engine", alloc }
            , _resources{ resources }
            , _input_module{ nullptr, { _allocator } }
            // Frames allocators
            , _frame_allocator{ _allocator, sizeof(MemoryFrame) * 5 }
            , _frame_data_allocator{ { _allocator, detail::FrameAllocatorCapacity }, { _allocator, detail::FrameAllocatorCapacity } }
            // Frames
            , _previous_frame{ core::memory::make_unique<MemoryFrame>(_frame_allocator, _frame_data_allocator[0]) }
            , _current_frame{ core::memory::make_unique<MemoryFrame>(_frame_allocator, _frame_data_allocator[1]) }
        {
            auto* sdl_driver_module_location = resources.find({ "sdl2_driver.dll" });
            IS_ASSERT(sdl_driver_module_location != nullptr, "Missing SDL2 driver module!");

            _input_module = input::load_driver_module(_allocator, sdl_driver_module_location->location().path);
        }

        ~IceShardEngine() noexcept
        {
            _current_frame = nullptr;
            _previous_frame = nullptr;
        }


        auto revision() const noexcept -> uint32_t override
        {
            return 1;
        }

        auto input_system() const noexcept -> input::InputSystem*
        {
            return _input_module->input_system();
        }

        auto previous_frame() const noexcept -> const Frame& override
        {
            return *_previous_frame;
        }

        auto current_frame() noexcept -> Frame& override
        {
            return *_current_frame;
        }

        void next_frame() noexcept override
        {
            cppcoro::sync_wait(
                cppcoro::when_all_ready(std::move(_frame_tasks))
            );


            // Move the current frame to the 'previous' slot.
            _previous_frame = std::move(_current_frame);

            // Reset the frame allocator inner pointers.
            [[maybe_unused]]
            const bool successful_reset = _frame_data_allocator[_next_free_allocator].reset();
            IS_ASSERT(successful_reset == true, "Memory was discarded during frame allocator reset!");

            _current_frame = core::memory::make_unique<MemoryFrame>(_frame_allocator, _frame_data_allocator[_next_free_allocator]);


            // We need to update the allocator index
            _next_free_allocator += 1;
            _next_free_allocator %= detail::array_element_count(_frame_data_allocator);


            // Now we want to get all messages for the current frame.
            auto* inputs = input_system();
            inputs->query_messages(_current_frame->messages());
        }

        void add_task(cppcoro::task<> task) noexcept override
        {
            _frame_tasks.push_back(std::move(task));
        }

    private:
        core::memory::proxy_allocator _allocator;

        // Resource systems.
        resource::ResourceSystem& _resources;

        // Input system.
        core::memory::unique_pointer<input::InputModule> _input_module;

        // Tasks to be run this frame.
        std::vector<cppcoro::task<>> _frame_tasks;

        // Frame allocators.
        uint32_t _next_free_allocator = 0;

        core::memory::scratch_allocator _frame_allocator;
        core::memory::scratch_allocator _frame_data_allocator[2];

        // Frames.
        core::memory::unique_pointer<MemoryFrame> _previous_frame;
        core::memory::unique_pointer<MemoryFrame> _current_frame;
    };

}

extern "C"
{

    __declspec(dllexport) auto create_engine(core::allocator& alloc, resource::ResourceSystem& resources) -> iceshard::Engine*
    {
        return alloc.make<iceshard::IceShardEngine>(alloc, resources);
    }

    __declspec(dllexport) void release_engine(core::allocator& alloc, iceshard::Engine* engine)
    {
        alloc.destroy(engine);
    }

}
