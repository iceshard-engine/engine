/// Copyright 2025 - 2025, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/devui_widget.hxx>
#include <ice/array.hxx>
#include <atomic>

#include "iceshard_world_tasks_launcher.hxx"

namespace ice
{

    struct TraitTasksSnapshot
    {
        struct EventEntry
        {
            ice::Timestamp timestamp;
            ice::u32 type;
            ice::u32 next;
        };

        ice::Array<EventEntry> _events;
        bool _release;

        TraitTasksSnapshot(ice::Allocator& alloc, ice::Span<EventEntry const> events) noexcept
            : _events{ alloc }
            , _release{ false }
        {
            _events.push_back(events);
        }

        void draw() noexcept;
    };

    class TraitTasksTrackerDevUI : public ice::DevUIWidget, public ice::detail::TraitTaskTracker
    {
    public:
        TraitTasksTrackerDevUI(ice::Allocator& alloc) noexcept;

    public: // Implementation of: ice::detail::TraitTaskTracker
        auto report_resume(ice::u32 id) noexcept -> ice::u32 override;
        // auto report_resume(ice::u32 id) noexcept -> ice::u32 override;
        auto report_suspend(ice::u32 id) noexcept -> ice::u32 override;
        void report_finish(ice::u32 id) noexcept override;

    public: // Implementation of: ice::DevUIWidget
        void update_state(ice::DevUIWidgetState& state) noexcept override;
        void build_content() noexcept override;

    private:
        ice::Allocator& _allocator;
        std::atomic_uint16_t _current_event_count;

        ice::u32 _stat_events;
        ice::Array<TraitTasksSnapshot::EventEntry> _tracked_task_events;
        ice::Array<TraitTasksSnapshot> _snapshots;
    };

} // namespace ice
