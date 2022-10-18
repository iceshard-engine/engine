#pragma once
#include <ice/engine_types.hxx>
#include <ice/ui_element_info.hxx>
#include <ice/ui_element_draw.hxx>
#include <ice/ui_element.hxx>
#include <ice/ui_resource.hxx>

#include <ice/asset.hxx>
#include <ice/font.hxx>
#include <ice/task.hxx>

namespace ice
{

    struct GameUI_ElementState
    {
        ice::ui::UpdateResult update_result;
    };

    class GameUI_Page
    {
    public:
        GameUI_Page(
            ice::Allocator& alloc,
            ice::Asset page_asset,
            ice::String page_asset_name
        ) noexcept;

        ~GameUI_Page() noexcept;

        auto name() const noexcept -> ice::String;
        auto name_hash() const noexcept -> ice::u64;

        auto info() const noexcept -> ice::ui::PageInfo const&;
        bool visible() const noexcept { return has_all(_current_flags, Flags::StateVisible); }

        auto element(ice::u16 idx) const noexcept -> ice::ui::Element const&;

        void open(ice::vec2u canvas_size) noexcept;

        void open_inside(
            ice::GameUI_Page const* parent_page,
            ice::ui::Element const* parent_element
        ) noexcept;

        void resize(
            ice::vec2u canvas_size
        ) noexcept;

        void close() noexcept;

        bool set_resource(
            ice::u32 resource_idx,
            ice::String string
        ) noexcept;

        bool set_resource(
            ice::u32 resource_idx,
            ice::Font const* font
        ) noexcept;

        template<typename T>
        bool set_resource(
            ice::StringID_Arg name,
            T value
        ) noexcept
        {
            ice::u32 idx = 0;
            for (ice::ui::UIResourceData& resource : _resources)
            {
                if (resource.info.id == name)
                {
                    return set_resource(idx, value);
                }
                idx += 1;
            }
            return false;
        }

        void set_dirty_layout() noexcept;
        void set_dirty_style() noexcept;

        void set_element_state(
            ice::ui::Element const& element,
            ice::ui::ElementState state
        ) noexcept;

        auto update(
            ice::EngineRunner& runner
        ) noexcept -> ice::Task<>;

    private:
        auto update_layout() noexcept -> ice::Task<>;
        auto update_style() noexcept -> ice::Task<>;
        auto update_resources(ice::EngineFrame const& frame) noexcept -> ice::Task<>;
        auto draw_text(ice::EngineFrame& frame) noexcept -> ice::Task<>;

    public:
        enum class Flags : ice::u8
        {
            None = 0x00,

            ActionShow = 0x01,
            ActionHide = 0x02,

            StateDirtyLayout = 0x10,
            StateDirtyStyle = 0x20,
            StateVisible = 0x40,
            StateInRenderMemory = 0x80,

            All = None
                | ActionShow | ActionHide
                | StateDirtyLayout | StateDirtyStyle
                | StateVisible | StateInRenderMemory
        };

    private:
        ice::Allocator& _allocator;
        ice::Asset const _asset;
        ice::String const _asset_name;
        ice::ui::PageInfo const* _page;

        ice::Memory _page_memory;
        ice::Span<ice::GameUI_ElementState> _states;
        ice::Span<ice::ui::Element> _elements;
        ice::Span<ice::ui::UIResourceData> _resources;
        ice::ui::DrawData _draw_data;

        ice::vec2f _current_canvas_size;
        ice::GameUI_Page const* _current_parent;
        ice::ui::Element const* _current_parent_element;

        Flags _current_flags;
    };

}
