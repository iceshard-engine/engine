
#pragma once
#include <ice/devui_widget.hxx>
#include <ice/devui_frame.hxx>
#include <ice/container/array.hxx>
#include <ice/log_sink.hxx>

namespace ice::devui
{

    struct ImGuiLogEntry
    {
        ice::LogSeverity severity;
        ice::LogTag tag;
        ice::String tagname;
        ice::HeapString<> message;
        ice::f64 filter_match = 1.0;
    };

    class ImGuiLogger final : public ice::DevUIWidget
    {
    public:
        ImGuiLogger(ice::Allocator& alloc) noexcept;
        ~ImGuiLogger() noexcept;

        void add_entry(ice::LogSinkMessage const& message) noexcept;

        void build_content() noexcept override;

    private:
        ice::Array<ImGuiLogEntry> _entries;
        ice::Array<ice::u32> _entries_visible;
    };

} // namespace ice::devui
