#include "iceshard_world_portal.hxx"

namespace ice
{

    namespace detail
    {

        struct WorldTraitTask
        {
            std::coroutine_handle<> _handle;

            WorldTraitTask(std::coroutine_handle<> handle) noexcept
                : _handle{ handle }
            {
            }

            struct promise_type
            {
                auto initial_suspend() const noexcept { return std::suspend_never{ }; }
                auto final_suspend() const noexcept { return std::suspend_always{ }; }
                auto return_void() noexcept { }

                auto get_return_object() noexcept;
                void unhandled_exception() noexcept
                {
                    ICE_ASSERT(false, "Unhandled coroutine exception!");
                }
            };
        };

        auto WorldTraitTask::promise_type::get_return_object() noexcept
        {
            return WorldTraitTask{ std::coroutine_handle<promise_type>::from_promise(*this) };
        }

    } // namespace detail

    IceshardWorldPortal::IceshardWorldPortal(
        ice::Allocator& alloc,
        ice::World const& world,
        ice::WorldTrait* trait,
        ice::ecs::EntityStorage& entity_storage,
        bool is_owning
    ) noexcept
        : _allocator{ alloc }
        , _world{ world }
        , _trait{ trait }
        , _is_owning{ is_owning }
        , _storage{ _allocator }
        , _entity_storage{ entity_storage }
        , _wait_event_allocator{ _allocator, { 512_B } }
        , _trait_tasks{ _allocator }
    {
        ice::array::reserve(_trait_tasks, 20);
    }

    IceshardWorldPortal::~IceshardWorldPortal() noexcept
    {
        for (TraitTask& trait_task : _trait_tasks)
        {
            trait_task.coroutine.destroy();
            _wait_event_allocator.destroy(trait_task.event);
        }
    }

    auto IceshardWorldPortal::world() const noexcept -> ice::World const&
    {
        return _world;
    }

    auto IceshardWorldPortal::trait() noexcept -> ice::WorldTrait*
    {
        return _trait;
    }

    auto IceshardWorldPortal::allocator() noexcept -> ice::Allocator&
    {
        return _allocator;
    }

    auto IceshardWorldPortal::storage() noexcept -> ice::DataStorage&
    {
        return _storage;
    }

    auto IceshardWorldPortal::entity_storage() noexcept -> ice::ecs::EntityStorage&
    {
        return _entity_storage;
    }

    void IceshardWorldPortal::execute(ice::Task<void> task) noexcept
    {
        ice::ManualResetEvent* wait_event = _wait_event_allocator.create<ManualResetEvent>();
        detail::WorldTraitTask trait_task = [](ice::Task<void> task, ice::ManualResetEvent* event) noexcept -> detail::WorldTraitTask
        {
            co_await task;
            event->set();
        }(ice::move(task), wait_event);

        ice::array::push_back(
            _trait_tasks,
            TraitTask{
                .event = wait_event,
                .coroutine = trait_task._handle
            }
        );
    }

    void IceshardWorldPortal::remove_finished_tasks() noexcept
    {
        ice::Array<TraitTask> running_tasks{ _allocator };

        for (TraitTask const& trait_task : _trait_tasks)
        {
            if (trait_task.event->is_set())
            {
                trait_task.coroutine.destroy();
                _wait_event_allocator.destroy(trait_task.event);
            }
            else
            {
                ice::array::push_back(running_tasks, trait_task);
            }
        }

        _trait_tasks = ice::move(running_tasks);
    }

} // namespace ice
