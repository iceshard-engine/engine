#include "engine_state_tracker_default.hxx"

namespace ice
{

    EngineStateTracker_Default::EngineStateTracker_Default(ice::Allocator& alloc) noexcept
        : _available_triggers{ alloc }
        , _initial_states{ alloc }
        , _state_committers{ alloc }
        , _current_state_index{ alloc }
        , _current_state{ alloc }
        , _pending_states{ alloc }
        , _subnames{ alloc }
    {
        ice::queue::reserve(_pending_states, 16);
    }

    auto create_state_tracker(ice::Allocator& alloc) noexcept -> ice::UniquePtr<ice::EngineStateTracker>
    {
        return ice::make_unique<ice::EngineStateTracker_Default>(alloc, alloc);
    }

} // namespace ice
