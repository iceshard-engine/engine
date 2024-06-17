/// Copyright 2024 - 2024, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#include "resource_provider_hailstorm_devui.hxx"
#include <ice/string/heap_string.hxx>
#include <ice/devui_context.hxx>
#include <ice/devui_frame.hxx>
#include <ice/devui_imgui.hxx>

#include <imgui/imgui.h>
#undef assert

namespace ice
{

    struct DevUIModules : ice::Module<DevUIModules>
    {
        static bool on_load(ice::Allocator& alloc, ice::ModuleNegotiator auto const& negotiator) noexcept
        {
            ice::devui_setup_context(negotiator);
            return true;
        }
    };

    // template<auto FieldPtr, bool UsesFormat>
    // constexpr void print_entry(void const* data, ice::String format) noexcept
    // {
    //     using Base = typename ice::member_info<decltype(FieldPtr)>::class_type;

    //     if constexpr (UsesFormat)
    //     {
    //         ImGui::Text(ice::string::begin(format), reinterpret_cast<Base const*>(data)->*FieldPtr);
    //     }
    //     else
    //     {
    //         ice::String const text = reinterpret_cast<Base const*>(data)->*FieldPtr;
    //         ImGui::TextUnformatted(ice::string::begin(text), ice::string::end(text));
    //     }
    // }

    // struct Entry
    // {
    //     using FnPrint = void(*)(void const* data, ice::String) noexcept;

    //     ice::String format;
    //     FnPrint fn_print;

    //     template<auto FieldPtr>
    //     static constexpr auto create() noexcept -> Entry
    //     {
    //         return {
    //             .fn_print{ print_entry<FieldPtr, false> }
    //         };
    //     }

    //     template<auto FieldPtr>
    //     static constexpr auto create(ice::String fmt_win, ice::String fmt_unix) noexcept -> Entry
    //     {
    //         return {
    //             .format = { ice::build::is_windows ? fmt_win : fmt_unix },
    //             .fn_print{ print_entry<FieldPtr, true> }
    //         };
    //     }
    // };

    auto create_hailstorm_provider_devui(
        ice::Allocator& alloc,
        ice::String name,
        ice::HailStormResourceProvider const& provider
    ) noexcept -> ice::UniquePtr<ice::DevUIWidget>
    {
        if (ice::devui_available())
        {
            return ice::make_unique<HailStormResourceProvider::DevUI>(alloc, ice::HeapString<>{ alloc, name }, provider);
        }
        return {};
    }

    HailStormResourceProvider::DevUI::DevUI(
        ice::HeapString<> name,
        ice::HailStormResourceProvider const& provider
    ) noexcept
        : DevUIWidget{ DevUIWidgetInfo{ .category = "Tools", .name = name } }
        , _name{ ice::move(name) } // Store the string so it's valid through the widget lifetime
        , _provider{ provider }
    {
        ice::devui_register_widget(this);
    }

    HailStormResourceProvider::DevUI::~DevUI() noexcept
    {
        ice::devui_remove_widget(this);
    }

    bool HailStormResourceProvider::DevUI::build_mainmenu(ice::DevUIWidgetState& state) noexcept
    {
        if (ImGui::BeginMenu("Resource Providers", true))
        {
            ImGui::MenuItem(ice::string::begin(info.name), nullptr, &state.active);
            ImGui::EndMenu();
        }
        return false;
    }

    void HailStormResourceProvider::DevUI::build_content() noexcept
    {
        IPT_ZONE_SCOPED;

        hailstorm::HailstormHeader const& header =  _provider._pack.header;
        ImGui::Text(
            "HailStorm v.%c%c%c%c (MAGIC: %c%c%c%c)",
            (char)((header.header_version >> 24) & 0xff),
            (char)((header.header_version >> 16) & 0xff),
            (char)((header.header_version >> 8) & 0xff),
            (char)((header.header_version >> 0) & 0xff),
            (char)((header.magic >> 24) & 0xff),
            (char)((header.magic >> 16) & 0xff),
            (char)((header.magic >> 8) & 0xff),
            (char)((header.magic >> 0) & 0xff)
        );
        ImGui::SetItemTooltip("Hailstorm data format version and MAGIC value.");
        if (ImGui::BeginChild("Pack Info", {}, ImGuiChildFlags_Border | ImGuiChildFlags_AutoResizeY))
        {
            ImGui::Text("Version: %hu.%hu.%hu", header.version[0], header.version[0], header.version[0]);
            ImGui::SetItemTooltip("Version of the DATA in the pack.");
            ImGui::TextT("Size: {:p}", ice::usize{header.offset_next});
            ImGui::SetItemTooltip("Size of the entire HailStorm pack before decompression and/or decryption.");

            ImGui::Text("Chunks: %hu", header.count_chunks);
            ImGui::Text("Resources: %hu", header.count_resources);
        }
        ImGui::EndChild();

        build_chunk_table();

        ImGui::TextUnformatted("Resources");
        if (ImGui::BeginChild("HailStormResourceProvider:Resources:Child"))
        {
            build_resources_table();
        }
        ImGui::EndChild();
    }

    static constexpr ImGuiTableFlags const Constant_TableCommonFlags = ImGuiTableFlags_None
        // Functional
        | ImGuiTableFlags_ContextMenuInBody
        | ImGuiTableFlags_Resizable
        | ImGuiTableFlags_Hideable
        // Visual
        | ImGuiTableFlags_HighlightHoveredColumn
        | ImGuiTableFlags_PadOuterX
        | ImGuiTableFlags_NoBordersInBody
        | ImGuiTableFlags_BordersOuterH
        | ImGuiTableFlags_BordersV
        | ImGuiTableFlags_RowBg;

    void HailStormResourceProvider::DevUI::build_chunk_table() noexcept
    {
        ImGui::TextUnformatted("Chunks");

        ice::u32 const entry_count = 6;
        ice::u32 const entry_size = static_cast<ice::u32>(ImGui::GetTextLineHeightWithSpacing());
        ice::u32 const table_size = entry_size * entry_count;

        if (ImGui::BeginTable("HailStormResourceProvider:Chunks", 7, Constant_TableCommonFlags, { 0, (ice::f32) table_size }))
        {
            ice::u32 const scroll_value = (ice::u32) ImGui::GetScrollY();
            [[maybe_unused]]
            ice::u32 const scroll_idx = scroll_value / entry_size;

            // Adjust the scroll position to always show full
            if (ice::u32 const scroll_offset = scroll_value % entry_size; scroll_offset > 0)
            {
                ImGui::SetScrollY(ImGui::GetScrollY() + ice::f32(entry_size - scroll_offset));
            }

            ImGui::TableSetupColumn("Index", ImGuiTableColumnFlags_NoHide);
            ImGui::TableSetupColumn("Type");
            ImGui::TableSetupColumn("Persistance");
            ImGui::TableSetupColumn("Entries");
            ImGui::TableSetupColumn("Size");
            ImGui::TableSetupColumn("Original Size", ImGuiTableColumnFlags_DefaultHide);
            ImGui::TableSetupColumn("File Offset", ImGuiTableColumnFlags_DefaultHide);
            ImGui::TableHeadersRow();

            constexpr char const* Constant_Types[]{ "AppDefined", "Metadata", "Data", "Mixed" };
            constexpr char const* Constant_Persistance[]{ "Temporary", "Regular", "Load If Possible", "Load Always" };

            // constexpr Entry Constant_FieldEntries[]{ Entry::create<&hailstorm::HailstormChunk::size>("%u", "%u") };

            ice::u32 idx = 0;
            for (hailstorm::HailstormChunk const& chunk : _provider._pack.chunks)
            {
                ImGui::TableNextRow();
                if (ImGui::TableNextColumn())
                {
                    ImGui::Text("%u", idx++);
                }
                if (ImGui::TableNextColumn()) // Type
                {
                    ImGui::TextUnformatted(Constant_Types[chunk.type]);
                }
                if (ImGui::TableNextColumn()) // Persistance
                {
                    ImGui::TextUnformatted(Constant_Persistance[chunk.persistance]);
                }
                if (ImGui::TableNextColumn()) // Entries
                {
                    ImGui::Text("%u", (ice::u32) chunk.count_entries);
                }
                if (ImGui::TableNextColumn()) // Size
                {
                    ImGui::TextT("{:p}", ice::usize{chunk.size});
                }
                if (ImGui::TableNextColumn()) // Original Size
                {
                    ImGui::TextT("{:p}", ice::usize{chunk.size_origin});
                }
                if (ImGui::TableNextColumn()) // File Offset
                {
                    ImGui::TextT("{:i}", ice::usize{chunk.offset});
                }
            }

            ImGui::EndTable();
        }
    }

    void HailStormResourceProvider::DevUI::build_resources_table() noexcept
    {
        ice::u32 const pack_name_size = ice::string::size(_name) + 1;

        ice::u32 const entry_count = 20;
        ice::u32 const entry_size = static_cast<ice::u32>(ImGui::GetTextLineHeightWithSpacing());
        ice::u32 const table_size = entry_size * entry_count;
        // ice::u32 const child_size = entry_size * _provider._pack.header.count_resources;
        // ImGui::SetWindowSize({ ImGui::GetWindowSize().x, (f32) child_size }, ImGuiCond_Always);

        if (ImGui::BeginTable("HailStormResourceProvider:Resources", 3, Constant_TableCommonFlags, { 0, (ice::f32) table_size }))
        {
            ice::u32 const scroll_value = (ice::u32) ImGui::GetScrollY();
            [[maybe_unused]]
            ice::u32 const scroll_idx = scroll_value / entry_size;

            // Adjust the scroll position to always show full
            if (ice::u32 const scroll_offset = scroll_value % entry_size; scroll_offset > 0)
            {
                ImGui::SetScrollY(ImGui::GetScrollY() + ice::f32(entry_size - scroll_offset));
            }

            ImGui::TableSetupColumn("Index", ImGuiTableColumnFlags_DefaultHide);
            ImGui::TableSetupColumn("Name", ImGuiTableColumnFlags_NoHide);
            ImGui::TableSetupColumn("Size");
            ImGui::TableHeadersRow();

            ice::u32 idx = 0;
            for (hailstorm::HailstormResource const& res : _provider._pack.resources)
            {
                ImGui::TableNextRow();
                if (ImGui::TableNextColumn())
                {
                    ImGui::Text("%u", idx++);
                }
                if (ImGui::TableNextColumn()) // Name
                {
                    char const* const path_str_beg = reinterpret_cast<char const*>(_provider._pack.paths_data.location) + res.path_offset;
                    char const* const path_str_end = path_str_beg + res.path_size;

                    // Strip the added prefix of the pack name
                    ImGui::TextUnformatted(path_str_beg + pack_name_size, path_str_end);
                }
                if (ImGui::TableNextColumn()) // Size
                {
                    ImGui::Text("%u", res.size);
                }
            }

            ImGui::EndTable();
        }
    }

} // namespace ice
