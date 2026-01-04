/// Copyright 2024 - 2025, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#include "imgui_logger.hxx"

#include <ice/color.hxx>
#include <ice/devui_imgui.hxx>
#include <ice/log.hxx>
#include <ice/math.hxx>
#include <ice/mem_allocator_host.hxx>
#include <ice/sort.hxx>
#include <ice/string/heap_string.hxx>
#include <ice/string_utils.hxx>

#include <rapidfuzz/distance.hpp>
#include <rapidfuzz/fuzz.hpp>
#include <mutex>

namespace ice::devui
{

    // TODO: Replace
    static ice::HostAllocator LoggerAlloc;

    struct ImGuiColorCtx
    {
        ice::u32 _pushed = 0;

        struct Entry
        {
            ImGuiCol target;
            ice::u32 color;
        };

        constexpr ImGuiColorCtx() noexcept = default;

        constexpr ImGuiColorCtx(ImGuiColorCtx&& other) noexcept
            : _pushed{ ice::exchange(other._pushed, 0) }
        {
        }

        constexpr auto operator=(ImGuiColorCtx&& other) noexcept -> ImGuiColorCtx&
        {
            _pushed = ice::exchange(other._pushed, 0);
            return *this;
        }

        inline ImGuiColorCtx(std::initializer_list<Entry> entries) noexcept
            : _pushed{ ice::u32(entries.size()) }
        {
            for (Entry entry : entries)
            {
                ImGui::PushStyleColor(entry.target, entry.color);
            }
        }

        inline ~ImGuiColorCtx() noexcept
        {
            ImGui::PopStyleColor(_pushed);
        }
    };

    ImGuiLogger::ImGuiLogger(ice::Allocator& a) noexcept
        : ice::DevUIWidget{ DevUIWidgetInfo{ .category = "Tools", .name = "Logger" } }
        , _entries{ LoggerAlloc }
        , _entries_visible{ LoggerAlloc }
    {
        ice::array::reserve(_entries, 2000);
        ice::array::reserve(_entries_visible, 2000);
    }

    ImGuiLogger::~ImGuiLogger() noexcept
    {
    }

    static std::mutex mtx; // TODO: Might want to replace the mutex with something else

    void ImGuiLogger::add_entry(ice::LogSinkMessage const& message) noexcept
    {
        std::lock_guard lk{ mtx };
        ice::array::push_back(_entries_visible, ice::array::count(_entries));
        ice::array::push_back(_entries, { message.severity, message.tag, message.tag_name, {LoggerAlloc,message.message} });
    }

    static inline auto severity_color(ice::LogSeverity sev) noexcept -> ImGuiColorCtx
    {
        switch(sev)
        {
        case LogSeverity::Critical:
        case LogSeverity::Error:
            return {
                { ImGuiCol_TableRowBg, ImGui::ToColor(0xAA'e83f48_argb) },
                { ImGuiCol_TableRowBgAlt, ImGui::ToColor(0xCC'e83f48_argb) }
            };
        case LogSeverity::Warning:
            return {
                { ImGuiCol_TableRowBg, ImGui::ToColor(0xAA'f6c409_argb) },
                { ImGuiCol_TableRowBgAlt, ImGui::ToColor(0xCC'f6c409_argb) }
            };
        case LogSeverity::Retail:
            // return {
            //     { ImGuiCol_TableRowBg, ImGui::ToColor(0xAA'7bd8ed_argb) },
            //     { ImGuiCol_TableRowBgAlt, ImGui::ToColor(0xCC'9ae1f1_argb) }
            // };
        case LogSeverity::Debug:
        case LogSeverity::Verbose:
            // return {
            //     { ImGuiCol_TableRowBg, ImGui::ToColor(0xAA'a68a9e_argb) },
            //     { ImGuiCol_TableRowBgAlt, ImGui::ToColor(0xCC'a68a9e_argb) }
            // };
        case LogSeverity::Info:
            // return {
            //     { ImGuiCol_TableRowBg, ImGui::ToColor(0xAA'a6bdd0_argb) },
            //     { ImGuiCol_TableRowBgAlt, ImGui::ToColor(0xCC'a6bdd0_argb) }
            // };
        default:
            return {};
        }
    }

    static constexpr auto severity_name(ice::LogSeverity sev) noexcept -> ice::String
    {
        switch(sev)
        {
        case LogSeverity::Critical: return "Critical";
        case LogSeverity::Retail: return "Retail";
        case LogSeverity::Error: return "Error";
        case LogSeverity::Warning: return "Warning";
        case LogSeverity::Info: return "Info";
        case LogSeverity::Verbose: return "Verbose";
        case LogSeverity::Debug: return "Debug";
        default: return "???";
        }
    }

    void ImGui_AddFilter() noexcept
    {
    }

    void ImGuiLogger::build_content() noexcept
    {
        ImGuiTableFlags const flags = ImGuiTableFlags_None
            // Functional
            | ImGuiTableFlags_ContextMenuInBody
            | ImGuiTableFlags_Resizable
            | ImGuiTableFlags_Hideable
            | ImGuiTableFlags_ScrollY
            | ImGuiTableFlags_Sortable
            | ImGuiTableFlags_SortMulti
            // Visual
            | ImGuiTableFlags_SizingStretchSame
            | ImGuiTableFlags_HighlightHoveredColumn
            | ImGuiTableFlags_PadOuterX
            | ImGuiTableFlags_NoBordersInBody
            // | ImGuiTableFlags_BordersInner
            | ImGuiTableFlags_BordersOuterH
            | ImGuiTableFlags_BordersV
            | ImGuiTableFlags_RowBg;

        static char filter_severity[16], filter_tag[32], filter_message[128];
        char* const filter_buffers[]{ filter_severity, filter_tag, filter_message };
        ice::u32 const filter_buffers_sizes[]{ ice::count(filter_severity), ice::count(filter_tag), ice::count(filter_message) };

        float const height = ImGui::GetContentRegionAvail().y;
        if (ImGui::BeginTable("logs", 3, flags, ImVec2{ 0.0f, height }))
        {
            ImGui::TableSetupColumn("Severity");
            ImGui::TableSetupColumn("Tag");
            ImGui::TableSetupColumn("Message");

            // NOTE: Building our own filters here
            // ImGui::TableHeadersRow();

            ImGui::PushTabStop(false);
            ImGui::TableNextRow(ImGuiTableRowFlags_Headers);
            for (int column = 0; column < 3; ++column)
            {
                ImGui::TableSetColumnIndex(column);
                const char* column_name = ImGui::TableGetColumnName(column); // Retrieve name passed to TableSetupColumn()
                ImGui::PushID(column);
                ImGui::TableHeader("");
                if (column >= 0)
                {
                    ImGui::SameLine(0.0f, ImGui::GetStyle().ItemInnerSpacing.x);
                    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0, 0));
                    ImGui::PushTabStop(true);
                    ImGui::InputTextWithHint(
                        "##filter",
                        column_name,
                        filter_buffers[column],
                        filter_buffers_sizes[column],
                        ImGuiInputTextFlags_EscapeClearsAll
                    );
                    ImGui::PopTabStop();
                    ImGui::PopStyleVar();
                }
                ImGui::PopID();
            }
            ImGui::PopTabStop();

            for (ImGuiLogEntry& entry : _entries)
            {
                entry.filter_match = rapidfuzz::fuzz::partial_token_ratio(
                    (ice::String) (char const*)filter_message,
                    (ice::String) entry.message
                );
            }

            std::lock_guard lk{ mtx };

            ice::f64 const max_match = _entries[_entries_visible[0]].filter_match;
            ice::sort_indices(
                ice::array::slice(_entries),
                ice::array::slice(_entries_visible),
                [max_match](ImGuiLogEntry const& l, ImGuiLogEntry const& r) noexcept
                {
                    if (l.filter_match == r.filter_match && r.filter_match >= max_match)
                    {
                        return rapidfuzz::levenshtein_normalized_distance(
                            (ice::String) (char const*)filter_message,
                            (ice::String) l.message
                        ) <= rapidfuzz::levenshtein_normalized_distance(
                            (ice::String) (char const*)filter_message,
                            (ice::String) r.message
                        );
                    }
                    return l.filter_match >= r.filter_match;
                }
            );

            ImGui::TableNextRow();

            for (ice::u32 idx : _entries_visible)
            {
                ImGuiLogEntry const& entry = _entries[idx];

                // if (entry.filter_match >= 0.0)
                {
                    ImGuiColorCtx color = severity_color(entry.severity);

                    if (ImGui::TableNextColumn()) // Severity
                    {
                        ImGui::TextT("{}", severity_name(entry.severity));
                    }
                    if (ImGui::TableNextColumn()) // Tag
                    {
                        ImGui::TextT("{}", entry.tagname);
                    }
                    if (ImGui::TableNextColumn()) // Message
                    {
                        ImGui::TextT("{}", (ice::String) entry.message);
                    }

                    ImGui::TableNextRow();
                }
            }

            ImGui::EndTable();
        }
    }

} // namespace ice::devui
