/// Copyright 2022 - 2023, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#include "game_ui_page.hxx"
#include "render_ui_trait.hxx"

#include <ice/game_ui.hxx>
#include <ice/game_render_traits.hxx>
#include <ice/engine_runner.hxx>
#include <ice/engine_frame.hxx>

#include <ice/ui_page.hxx>
#include <ice/ui_element.hxx>
#include <ice/ui_resource.hxx>
#include <ice/ui_style.hxx>
#include <ice/ui_data_utils.hxx>
#include <ice/ui_button.hxx>
#include <ice/ui_label.hxx>
#include <ice/ui_font.hxx>

#include <ice/mem_allocator_stack.hxx>
#include <ice/profiler.hxx>
#include <ice/font.hxx>

namespace ice
{

#if 0
    namespace detail
    {

        auto element_required_memory_size(
            ice::ui::ElementInfo const& element_info,
            ice::u32& count_verts_position,
            ice::u32& count_verts_color
        ) noexcept -> ice::meminfo
        {
            ice::meminfo result = ice::meminfo_of<ice::ui::Element>;
            result += ice::meminfo_of<ice::GameUI_ElementState>;
            if (element_info.style_i != 0)
            {
                result += ice::meminfo_of<ice::vec2f> * 4; // positions
                result += ice::meminfo_of<ice::vec4f> * 4; // colors
                count_verts_position += 4;
                count_verts_color += 4;
            }
            return result;
        }

        auto resource_required_memory_size(
            ice::ui::ResourceInfo const& resource_info
        ) noexcept -> ice::meminfo
        {
            ice::meminfo result = ice::meminfo_of<ice::ui::UIResourceData>;
            if (resource_info.type == ice::ui::ResourceType::String)
            {
                result += ice::meminfo_of<ice::String>;
                result += ice::meminfo{ resource_info.type_data + 1, ice::ualign::b_4 }; // We are not concerned about 1 byte overhead.
            }
            else if (resource_info.type == ice::ui::ResourceType::Font)
            {
                result += ice::meminfo_of<ice::Font const*>;
            }
            return result;
        }

        auto page_required_memory_size(
            ice::ui::PageInfo const& page_info,
            ice::u32& count_verts_position,
            ice::u32& count_verts_color
        ) noexcept -> ice::meminfo
        {
            ice::meminfo result = ice::meminfo_of<ice::Font const*>;
            for (ice::ui::ElementInfo const& element_info : page_info.elements)
            {
                result += element_required_memory_size(element_info, count_verts_position, count_verts_color);
            }
            for (ice::ui::ResourceInfo const& resource_info : page_info.ui_resources)
            {
                result += resource_required_memory_size(resource_info);
            }
            return result;
        }

        auto debug_required_memory_size(
            ice::ui::PageInfo const& page_info,
            ice::u32& count_verts
        ) noexcept -> ice::meminfo
        {
            ice::meminfo result{ };
            if constexpr (ice::build::is_release == false)
            {
                // Allocate enough space to draw debug boxes (bbox, hitbox, contentbox)
                ice::ucount const count_vertices_box = 4;
                ice::ucount const count_vertices_boxes = count_vertices_box * 3;
                ice::ucount const count_vertices_elements = count_vertices_boxes * ice::count(page_info.elements);
                count_verts += count_vertices_elements;

                result = ice::meminfo_of<ice::vec2f> * count_vertices_elements;
            }
            return result;
        }

        auto resource_set_value(
            ice::ui::UIResourceData& resource,
            ice::String str
        ) noexcept
        {
            ice::String* str_ptr = reinterpret_cast<ice::String*>(resource.location);

            ice::u32 const new_str_size = ice::min(ice::string::size(str), resource.info.type_data);
            if (new_str_size == 0)
            {
                *str_ptr = str;
            }
            else
            {
                char* str_data = reinterpret_cast<char*>(str_ptr + 1);
                ice::memcpy(
                    str_data,
                    ice::string::begin(str),
                    new_str_size
                );
                str_data[new_str_size] = '\0';
            }
        }

        auto resource_set_value(
            ice::ui::UIResourceData& resource,
            ice::Font const* font
        ) noexcept
        {
            Font const** font_ptr = reinterpret_cast<ice::Font const**>(resource.location);
            *font_ptr = font;
        }

    } // namespace detail


    GameUI_Page::GameUI_Page(
        ice::Allocator& alloc,
        ice::Asset page_asset,
        ice::Data page_data,
        ice::String page_asset_name
    ) noexcept
        : _allocator{ alloc }
        , _asset{ ice::move(page_asset) }
        , _asset_name{ page_asset_name }
        , _page{ reinterpret_cast<ice::ui::PageInfo const*>(page_data.location) }
        , _page_memory{ }
        , _states{ }
        , _elements{ }
        , _resources{ }
        , _draw_data{ }
        , _current_canvas_size{ 0 }
        , _current_parent{ nullptr }
        , _current_parent_element{ nullptr }
        , _current_flags{ }
    {
        ice::u32 count_verts_debug = 0;
        ice::meminfo const debug_data_size = ice::detail::debug_required_memory_size(*_page, count_verts_debug);

        ice::u32 count_verts_position = 0;
        ice::u32 count_verts_color = 0;

        ice::meminfo mi_page = ice::detail::page_required_memory_size(*_page, count_verts_position, count_verts_color);
        /*ice::usize const off_debugdata = */mi_page += debug_data_size;

        _page_memory = _allocator.allocate(mi_page);
        ice::memset(_page_memory.location, 0, _page_memory.size.value);

        ice::u32 const count_elements = ice::count(_page->elements);
        ice::u32 const count_resources = ice::count(_page->ui_resources);

        ice::GameUI_ElementState* states_ptr = reinterpret_cast<ice::GameUI_ElementState*>(_page_memory.location);
        ice::ui::Element* const elements_ptr = reinterpret_cast<ice::ui::Element*>(
            ice::align_to(states_ptr + count_elements, ice::align_of<ice::ui::Element>).value
        );
        static_assert(alignof(ice::ui::UIResourceData) == alignof(ice::ui::Element));
        ice::ui::UIResourceData* const resources_ptr = reinterpret_cast<ice::ui::UIResourceData*>(elements_ptr + count_elements);

        ice::vec2f* positions_ptr = reinterpret_cast<ice::vec2f*>(resources_ptr + count_resources);
        ice::vec4f* colors_ptr = reinterpret_cast<ice::vec4f*>(positions_ptr + count_verts_position);

        void* additional_data_ptr = colors_ptr + count_verts_color;

        // Setup global data
        _states = { states_ptr, count_elements };
        _elements = { elements_ptr, count_elements };
        _resources = { resources_ptr, count_resources };

        _draw_data.vertice_count = count_verts_color;
        _draw_data.vertices = { positions_ptr, count_verts_position };
        _draw_data.colors = { colors_ptr, count_verts_color };

        // Update element infos
        ice::u32 idx = 0;
        for (ice::ui::ElementInfo const& element_info : _page->elements)
        {
            _states[idx] = { };

            ice::ui::Element& parent = elements_ptr[element_info.parent];
            ice::ui::Element& element = elements_ptr[idx];
            element.definition = &element_info;

            if (element_info.style_i != 0)
            {
                element.draw_data.vertice_count = 4;
                element.draw_data.vertices = { positions_ptr, 4 };
                element.draw_data.colors = { colors_ptr, 4 };

                positions_ptr += 4;
                colors_ptr += 4;
            }

            if (parent.child == nullptr)
            {
                // Dont self-assign
                if (&parent != &element)
                {
                    parent.child = &element;
                }
            }
            else
            {
                ice::ui::Element* sibling = parent.child;
                while (sibling->sibling != nullptr)
                {
                    sibling = sibling->sibling;
                }
                sibling->sibling = &element;
            }

            idx += 1;
        }

        // Update resource infos
        for (idx = 0; idx < count_resources; ++idx)
        {
            ice::ui::UIResourceData& resource = _resources[idx];
            resource.info = _page->ui_resources[idx];

            if (resource.info.type == ice::ui::ResourceType::String)
            {
                resource.location = ice::align_to(
                    additional_data_ptr,
                    ice::align_of<ice::String>
                ).value;

                additional_data_ptr = ice::ptr_add(
                    resource.location,
                    ice::size_of<ice::String> + ice::usize{ resource.info.type_data + 1 }
                );

                ice::detail::resource_set_value(resource, "");
            }
            else if (resource.info.type == ice::ui::ResourceType::Font)
            {
                resource.location = ice::align_to(
                    additional_data_ptr,
                    ice::align_of<ice::Font const*>
                ).value;
                additional_data_ptr = ice::ptr_add(
                    resource.location,
                    ice::size_of<ice::Font const*>
                );
            }
        }

        // TODO:
        if constexpr (ice::build::is_release == false)
        {
            void* debug_data = ice::ptr_add(_page_memory.location, _page_memory.size);
            ICE_ASSERT(additional_data_ptr <= debug_data, "Debug data starts at runtime memory location!");
        }
    }

    GameUI_Page::~GameUI_Page() noexcept
    {
        _allocator.deallocate(_page_memory);
    }

    auto GameUI_Page::name() const noexcept -> ice::String
    {
        return _asset_name;
    }

    auto GameUI_Page::name_hash() const noexcept -> ice::u64
    {
        return ice::hash(_asset_name);
    }

    auto GameUI_Page::info() const noexcept -> ice::ui::PageInfo const&
    {
        return *_page;
    }

    auto GameUI_Page::element(ice::u16 idx) const noexcept -> ice::ui::Element const&
    {
        ICE_ASSERT(idx <= ice::count(_elements), "Out of bounds");
        return _elements[idx];
    }

    void GameUI_Page::open(ice::vec2u canvas_size) noexcept
    {
        _current_canvas_size = { ice::f32(canvas_size.x), ice::f32(canvas_size.y) };
        _current_flags |= Flags::ActionShow | Flags::StateDirtyLayout | Flags::StateDirtyStyle;
        _current_flags &= ~Flags::ActionHide;
    }

    void GameUI_Page::open_inside(
        ice::GameUI_Page const* parent_page,
        ice::ui::Element const* parent_element
    ) noexcept
    {
        ICE_ASSERT(
            parent_page != nullptr && parent_element != nullptr,
            "Opening inside a different page, requires both the page and the owning element."
        );

        _current_parent = parent_page;
        _current_parent_element = parent_element;
        _current_flags |= Flags::ActionShow | Flags::StateDirtyLayout | Flags::StateDirtyStyle;
        _current_flags &= ~Flags::ActionHide;
    }

    void GameUI_Page::resize(ice::vec2u canvas_size) noexcept
    {
        _current_canvas_size = { ice::f32(canvas_size.x), ice::f32(canvas_size.y) };
        _current_flags |= Flags::StateDirtyLayout | Flags::StateDirtyStyle;
    }

    void GameUI_Page::close() noexcept
    {
        _current_flags |= Flags::ActionHide;
        _current_flags &= ~Flags::ActionShow;
    }

    bool GameUI_Page::set_resource(
        ice::u32 resource_idx,
        ice::String string
    ) noexcept
    {
        using ice::ui::ResourceType;
        if (resource_idx >= ice::count(_resources))
        {
            return false;
        }

        ice::ui::UIResourceData& resource = _resources[resource_idx];
        ICE_ASSERT(resource.info.type == ResourceType::String, "Trying to set incompatible value to resource!");
        ice::detail::resource_set_value(resource, string);
        return true;
    }

    bool GameUI_Page::set_resource(
        ice::u32 resource_idx,
        ice::Font const* font
    ) noexcept
    {
        using ice::ui::ResourceType;
        if (resource_idx >= ice::count(_resources))
        {
            return false;
        }

        ice::ui::UIResourceData& resource = _resources[resource_idx];
        ICE_ASSERT(resource.info.type == ResourceType::Font, "Trying to set incompatible value to resource!");
        ice::detail::resource_set_value(resource, font);
        return true;
    }

    void GameUI_Page::set_dirty_layout() noexcept
    {
        _current_flags |= Flags::StateDirtyLayout;
    }

    void GameUI_Page::set_dirty_style() noexcept
    {
        _current_flags |= Flags::StateDirtyStyle;
    }

    void GameUI_Page::set_element_state(
        ice::ui::Element const& element,
        ice::ui::ElementState state
    ) noexcept
    {
        ice::u16 const idx = ice::u16(&element - ice::span::data(_elements));
        ICE_ASSERT(idx < ice::count(_elements), "Out of bounds!");
        if (_elements[idx].state != state)
        {
            set_dirty_style();
            _elements[idx].state = state;
        }
    }

    auto GameUI_Page::update(
        ice::EngineRunner& runner
    ) noexcept -> ice::Task<>
    {
        if (has_any(_current_flags, Flags::StateDirtyStyle | Flags::StateDirtyLayout))
        {
            co_await runner.task_scheduler();

            if (has_all(_current_flags, Flags::StateDirtyLayout))
            {
                co_await update_layout();
            }

            if (has_all(_current_flags, Flags::StateDirtyStyle))
            {
                co_await update_style();
            }
        }

        ice::EngineFrame& frame = co_await runner.stage_current_frame();

        co_await update_resources(frame);

        if (has_any(_current_flags, Flags::StateDirtyStyle | Flags::StateDirtyLayout))
        {
            ice::RenderUIRequestType type = RenderUIRequestType::CreateOrUpdate;
            if (has_any(_current_flags, Flags::ActionShow))
            {
                type = RenderUIRequestType::UpdateAndShow;
                _current_flags |= Flags::StateVisible;
                _current_flags &= ~Flags::ActionShow;
            }
            if (has_any(_current_flags, Flags::ActionHide))
            {
                type = RenderUIRequestType::UpdateAndHide;
                _current_flags &= ~(Flags::StateVisible | Flags::ActionHide);
            }

            ice::ui::Position const pos = ice::ui::rect_position(_elements[0].bbox);
            ice::RenderUIRequest const* const request = frame.storage().create_named_object<ice::RenderUIRequest>(
                ice::stringid(_asset_name),
                ice::RenderUIRequest
                {
                    .id = name_hash(),
                    .position = { pos.x, pos.y },
                    .draw_data = &_draw_data,
                    .type = type
                }
            );

            ice::shards::push_back(frame.shards(), ice::Shard_RenderUIData | request);
            _current_flags |= Flags::StateInRenderMemory;
        }

        if (has_all(_current_flags, Flags::StateVisible))
        {
            if (has_all(_current_flags, Flags::ActionHide))
            {
                ice::RenderUIRequest const* const request = frame.storage().create_named_object<ice::RenderUIRequest>(
                    ice::stringid(_asset_name),
                    ice::RenderUIRequest
                    {
                        .id = name_hash(),
                        .type = RenderUIRequestType::Disable
                    }
                );

                ice::shards::push_back(frame.shards(), ice::Shard_RenderUIData | request);
                _current_flags &= ~Flags::StateVisible;
            }
            else
            {
                co_await draw_text(frame);
            }
        }
        else if (has_all(_current_flags, Flags::ActionShow))
        {
            ice::RenderUIRequest const* const request = frame.storage().create_named_object<ice::RenderUIRequest>(
                ice::stringid(_asset_name),
                ice::RenderUIRequest
                {
                    .id = name_hash(),
                    .type = RenderUIRequestType::Enable
                }
            );

            ice::shards::push_back(frame.shards(), ice::Shard_RenderUIData | request);
            _current_flags |= Flags::StateVisible;
        }

        // Remove all handled flags.
        _current_flags &= ~(Flags::StateDirtyStyle | Flags::StateDirtyLayout | Flags::ActionShow | Flags::ActionHide);
        co_return;
    }

    auto GameUI_Page::update_layout() noexcept -> ice::Task<>
    {
        using ice::ui::UpdateStage;
        using ice::ui::UpdateResult;
        using ice::ui::ElementFlags;

        auto const contains_unresolved = [this]() noexcept
        {
            for (ice::GameUI_ElementState const& result : _states)
            {
                if (result.update_result == UpdateResult::Unresolved)
                {
                    return true;
                }
            }
            return false;
        };

        IPT_ZONE_SCOPED;
        ice::u32 const count_elements = ice::count(_page->elements);

        for (ice::u32 idx = 0; idx < count_elements; ++idx)
        {
            ice::ui::ElementInfo const& current_element_data = _page->elements[idx];
            ice::ui::Element& current_element = _elements[idx];
            ice::ui::Element& parent_element = _elements[current_element_data.parent];

            _states[idx].update_result = ice::ui::element_update(
                UpdateStage::ExplicitSize,
                *_page,
                parent_element,
                current_element_data,
                current_element,
                _resources
            );
        }

        if (_current_parent == nullptr)
        {
            _elements[0].flags = ElementFlags::None;
            _elements[0].bbox = ice::ui::Rect{
                .left = 0,
                .top = 0,
                .right = _current_canvas_size.x,
                .bottom = _current_canvas_size.y,
            };
        }
        else
        {
            _elements[0].flags = ElementFlags::None;
            _elements[0].bbox = _current_parent_element->contentbox;
        }

        _elements[0].hitbox = _elements[0].bbox;
        _elements[0].contentbox = _elements[0].hitbox;

        // Calculate auto sizes (childs to parents)
        if (contains_unresolved())
        {
            for (ice::i32 idx = count_elements - 1; idx >= 0; --idx)
            {
                if (_states[idx].update_result == UpdateResult::Resolved)
                {
                    continue;
                }

                ice::ui::Element& current_element = _elements[idx];
                ice::ui::Element& parent_element = _elements[current_element.definition->parent];

                _states[idx].update_result = ice::ui::element_update(
                    UpdateStage::AutoSize,
                    *_page,
                    parent_element,
                    *current_element.definition,
                    current_element,
                    _resources
                );
            }
        }

        // Calculate stretch sizes (childs to siblings)
        while (contains_unresolved())
        {
            for (ice::i32 idx = count_elements - 1; idx >= 0; --idx)
            {
                if (_states[idx].update_result == UpdateResult::Resolved)
                {
                    continue;
                }

                ice::ui::Element& current_element = _elements[idx];
                ice::ui::Element& parent_element = _elements[current_element.definition->parent];

                _states[idx].update_result = ice::ui::element_update(
                    UpdateStage::StretchSize,
                    *_page,
                    parent_element,
                    *current_element.definition,
                    current_element,
                    _resources
                );
            }
        }

        // Calculate positions and some remaining sizes
        for (ice::u32 idx = 0; idx < count_elements; ++idx)
        {
            ice::ui::Element& current_element = _elements[idx];
            ice::ui::Element& parent_element = _elements[current_element.definition->parent];

            _states[idx].update_result = ice::ui::element_update(
                UpdateStage::Position,
                *_page,
                parent_element,
                *current_element.definition,
                current_element,
                _resources
            );
        }

        ICE_ASSERT(
            contains_unresolved() == false,
            "UI sizes should be fully resolved!"
        );
        co_return;
    }

    auto GameUI_Page::update_style() noexcept -> ice::Task<>
    {
        using ice::ui::StyleFlags;

        IPT_ZONE_SCOPED;

        for (ice::ui::Element const& element : _elements)
        {
            ice::ui::StyleColor const* color_data = nullptr;
            if (ice::ui::element_get_style(*_page, element, StyleFlags::TargetBackground, color_data))
            {
                ice::vec2f* vertices = ice::span::data(element.draw_data.vertices);
                ice::vec4f* colors = ice::span::data(element.draw_data.colors);

                // Colors
                ice::vec4f const style_color = ice::vec4f{
                    color_data->red,
                    color_data->green,
                    color_data->blue,
                    color_data->alpha
                };

                colors[0] = colors[1] = colors[2] = colors[3] = style_color;

                ice::ui::Position const pos = ice::ui::rect_position(element.hitbox);
                ice::ui::Size const size = ice::ui::rect_size(element.hitbox);

                // Positions (TriFan)
                // [0   2]
                // [1   3]
                vertices[0].x = pos.x;
                vertices[0].y = pos.y;
                vertices[1].x = pos.x;
                vertices[1].y = pos.y + size.height;
                vertices[2].x = pos.x + size.width;
                vertices[2].y = pos.y;
                vertices[3].x = pos.x + size.width;
                vertices[3].y = pos.y + size.height;
            }
        }

        co_return;
    }

    auto GameUI_Page::update_resources(ice::EngineFrame const& frame) noexcept -> ice::Task<>
    {
        using ice::ui::ResourceType;
        ice::StackAllocator<512_B> alloc;

        ice::Array<ice::UpdateUIResource const*> resource_updates{ alloc };
        if (ice::shards::inspect_all(frame.shards(), ice::Shard_GameUI_UpdateResource, resource_updates))
        {
            for (ice::UpdateUIResource const* resupdate : resource_updates)
            {
                ice::u64 const name_hash = ice::hash(resupdate->page);
                if (this->name_hash() == name_hash)
                {
                    if (resupdate->resource_type == ResourceType::String)
                    {
                        this->set_resource(
                            resupdate->resource,
                            *reinterpret_cast<ice::String const*>(resupdate->resource_data)
                        );
                    }
                    else if (resupdate->resource_type == ResourceType::Font)
                    {
                        this->set_resource(
                            resupdate->resource,
                            reinterpret_cast<ice::Font const*>(resupdate->resource_data)
                        );
                    }

                    _current_flags |= Flags::StateDirtyLayout;
                }
            }
        }
        co_return;
    }

    auto GameUI_Page::draw_text(ice::EngineFrame& frame) noexcept -> ice::Task<>
    {
        if (frame.storage().named_object<ice::GameUI_Page*>(stringid(name())) != nullptr)
        {
            co_return;
        }

        frame.storage().create_named_object<ice::GameUI_Page*>(stringid(name()), this);

        ice::u32 idx = 0;
        for (ice::ui::Element const& element : _elements)
        {
            ice::ui::ElementInfo const& element_info = *element.definition;
            ice::ui::FontInfo const* font_info = nullptr;
            ice::String text;

            ice::u32 type_idx = element_info.type_data_i + idx++;
            if (element_info.type == ui::ElementType::Button)
            {
                ice::ui::ButtonInfo const& button_info = _page->data_buttons[element_info.type_data_i];
                font_info = &_page->fonts[button_info.font.source_i];
                text = ice::ui::element_get_text(*_page, button_info, _resources);
            }
            else if (element_info.type == ui::ElementType::Label)
            {
                ice::ui::LabelInfo const& label_info = _page->data_labels[element_info.type_data_i];
                font_info = &_page->fonts[label_info.font.source_i];
                text = ice::ui::element_get_text(*_page, label_info, _resources);
                type_idx += 256;
            }

            if (font_info != nullptr)
            {
                ice::DrawTextCommand* draw_text = frame.storage().create_named_object<ice::DrawTextCommand>(
                    ice::StringID{ ice::StringID_Hash{ name_hash() + type_idx } }
                );
                ice::String const font_name{
                    reinterpret_cast<char const*>(ice::ptr_add(_page->additional_data, { font_info->font_name_offset })),
                    font_info->font_name_size
                };

                draw_text->font = font_name;
                draw_text->font_size = font_info->font_size;
                draw_text->text = text;

                ice::ui::Position pos = ice::ui::rect_position(element.contentbox);
                ice::ui::Size size = ice::ui::rect_size(element.contentbox);
                draw_text->position = ice::vec2u{
                    (ice::u32)(pos.x),
                    (ice::u32)(pos.y + size.height)
                };

                ice::shards::push_back(
                    frame.shards(),
                    ice::Shard_DrawTextCommand | (ice::DrawTextCommand const*)draw_text
                );
            }
        }
        co_return;
    }
#endif

} // namespace ice
