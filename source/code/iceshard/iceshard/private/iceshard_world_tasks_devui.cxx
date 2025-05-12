/// Copyright 2025 - 2025, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#include "iceshard_world_tasks_devui.hxx"
#include <ice/devui_imgui.hxx>
#include <ice/devui_context.hxx>
#include <ice/clock.hxx>

namespace ice
{

    TraitTasksTrackerDevUI::TraitTasksTrackerDevUI(ice::Allocator& alloc) noexcept
        : ice::DevUIWidget{ {.category="Engine", .name="Tasks Tracker"} }
        , _allocator{ alloc }
        , _current_event_count{ 1 }
        , _tracked_task_events{ _allocator }
        , _snapshots{ _allocator }
    {
        ice::devui_register_widget(this);
        ice::array::resize(_tracked_task_events, 500);
    }

    auto TraitTasksTrackerDevUI::report_resume(ice::u32 id) noexcept -> ice::u32
    {
        ice::u32 const eventidx = _current_event_count.fetch_add(1, std::memory_order_relaxed);
        if (eventidx >= ice::count(_tracked_task_events))
        {
            return 0;
        }

        _tracked_task_events[id].next = eventidx;
        _tracked_task_events[eventidx] = {
            .timestamp = ice::clock::now(),
            .type = 1
        };
        return eventidx;
    }

    auto TraitTasksTrackerDevUI::report_suspend(ice::u32 id) noexcept -> ice::u32
    {
        ICE_ASSERT_CORE(false);
        return 0;
    }

    void TraitTasksTrackerDevUI::report_finish(ice::u32 id) noexcept
    {
        ice::u32 const eventidx = _current_event_count.fetch_add(1, std::memory_order_relaxed);
        if (eventidx >= ice::count(_tracked_task_events))
        {
            return;
        }

        _tracked_task_events[id].next = eventidx;
        _tracked_task_events[eventidx] = {
            .timestamp = ice::clock::now(),
            .type = 3
        };
    }

    void TraitTasksTrackerDevUI::update_state(ice::DevUIWidgetState &state) noexcept
    {
        _stat_events = _current_event_count.exchange(1, std::memory_order_relaxed);
        if (_stat_events >= ice::count(_tracked_task_events))
        {
            ice::array::grow(_tracked_task_events, _stat_events);
            ice::array::resize(_tracked_task_events, _stat_events);
        }
    }

    void TraitTasksTrackerDevUI::build_content() noexcept
    {
        ImGui::TextT("Collected events: {}", _stat_events - 1);
        if (ImGui::Button("Take snapshot"))
        {
            ice::array::push_back(_snapshots, TraitTasksSnapshot{ _allocator, ice::array::slice(_tracked_task_events, 0, _stat_events) });
        }

        for (ice::TraitTasksSnapshot& snapshot : _snapshots)
        {
            if (snapshot._release == true)
            {
                snapshot._release = false;
                ice::array::clear(snapshot._events);
            }
            else if (ice::array::any(snapshot._events))
            {
                snapshot.draw();
            }
        }

        _tracked_task_events[0].timestamp = ice::clock::now();
    }

    void TraitTasksSnapshot::draw() noexcept
    {
        // ImVec2 const cursor = ImGui::GetWindowContentRegionMin();
        ImVec2 const region = ImGui::GetWindowContentRegionMax();

        ice::u32 first_entry = 1;
        ice::u32 const last_entry = ice::count(_events);
        EventEntry const* entries = ice::begin(_events);

        ice::u32 running = 0;
        EventEntry const* concurrent[32]{};

        ice::Timestamp const startts = entries[first_entry].timestamp;
        ice::Timestamp const endts = entries[last_entry-1].timestamp;
        ice::Tns const duration = ice::clock::elapsed(startts, endts);
        ice::Ts const durantion_s = ice::Ts(duration) * 1.1;

        ImGui::Separator();
        ImGui::TextT("Total time: {}", duration); ImGui::SameLine();
        ImGui::PushID(this);
        if (ImGui::Button("Delete"))
        {
            _release = true;
        }
        ImGui::PopID();

        ImVec2 const cursor = ImGui::GetCursorScreenPos();

        ice::u32 max_running = 0;
        while(first_entry < last_entry)
        {
            EventEntry const* entry = entries + first_entry;
            switch(entry->type)
            {
            case 1: // Resume
                running = 0;
                while(concurrent[running] != nullptr) running += 1;
                concurrent[running] = entry;
                break;
            case 3: // Finish
            {
                running = 0;
                while(concurrent[running] == nullptr || concurrent[running]->next != first_entry)
                {
                    running += 1;
                    max_running = ice::max(max_running, running);
                }

                ice::Timestamp const to = entry->timestamp;
                ice::Timestamp const from = concurrent[running]->timestamp;

                // ImGui::TextT("{:n}", ice::clock::elapsed(from, to));

                ice::Ts const sb = ice::clock::elapsed(startts, from);
                ice::Ts const s = ice::clock::elapsed(from, to);

                ice::u32 const offset = ice::u32((sb / durantion_s).value * region.x);
                ice::u32 const size = ice::u32((s / durantion_s).value * region.x);

                ImVec2 const tl{ cursor.x + 10.0f + offset, cursor.y + 3 + running*12.0f };
                ImVec2 const br{ cursor.x + 10.0f + offset + size, cursor.y + 3 + running*12.0f + 8 };
                ImDrawList* drawlist = ImGui::GetWindowDrawList();
                drawlist->AddRectFilled(tl, br, ImGui::ToColor(0xFF'994466_argb));

                // Remove and swap places with last
                concurrent[running] = nullptr;
                break;
            }
            default: break;
            }

            first_entry += 1;
        }

        ImVec2 new_cursor{ cursor.x, cursor.y + 6 + 8 + max_running * 12 };
        ImGui::SetCursorScreenPos(new_cursor);
    }

} // namespace ice
