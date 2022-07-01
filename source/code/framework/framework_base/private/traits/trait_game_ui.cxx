#include "trait_game_ui.hxx"
#include "render/trait_render_ui.hxx"

#include <ice/game_ui.hxx>
#include <ice/game_render_traits.hxx>
#include <ice/ui_asset.hxx>
#include <ice/ui_element.hxx>
#include <ice/ui_element_info.hxx>
#include <ice/ui_button.hxx>

#include <ice/engine.hxx>
#include <ice/engine_frame.hxx>
#include <ice/engine_runner.hxx>
#include <ice/engine_shards.hxx>
#include <ice/world/world_portal.hxx>
#include <ice/world/world_trait_archive.hxx>

#include <ice/ecs/ecs_entity_index.hxx>
#include <ice/ecs/ecs_entity_operations.hxx>
#include <ice/ecs/ecs_archetype.hxx>
#include <ice/ecs/ecs_archetype_index.hxx>
#include <ice/ecs/ecs_entity_storage.hxx>

#include <ice/gfx/gfx_device.hxx>
#include <ice/render/render_swapchain.hxx>

#include <ice/input/input_event.hxx>
#include <ice/input/input_mouse.hxx>

#include <ice/platform_event.hxx>
#include <ice/stack_string.hxx>
#include <ice/shard_container.hxx>
#include <ice/task_scheduler.hxx>
#include <ice/task_thread_pool.hxx>
#include <ice/pod/array.hxx>
#include <ice/asset_storage.hxx>

namespace ice
{

    constexpr ice::u32 Constant_DebugBoundingBox = 0;
    constexpr ice::u32 Constant_DebugContentBox = 0;

    namespace detail
    {

        inline auto make_empty_entity_handle(
            ice::ecs::Entity entity
        ) noexcept -> ice::ecs::EntityHandle
        {
            ice::ecs::EntityHandleInfo const info{
                    .entity = entity,
                    .slot = ice::ecs::EntitySlot::Invalid
            };
            return std::bit_cast<ice::ecs::EntityHandle>(info);
        }
    }

    struct UIPage
    {
        static constexpr ice::StringID Identifier = "ice.component.ui-page"_sid;

        ice::u64 page_hash;
    };

    struct UIElement
    {
        static constexpr ice::StringID Identifier = "ice.component.ui-element"_sid;

        //ice::ecs::Entity page_entity;
        ice::u64 page_hash;
        ice::u8 element_idx;
    };

    struct UIButton
    {
        static constexpr ice::StringID Identifier = "ice.component.ui-button"_sid;

        ice::ui::Action const* action_on_click;
        ice::ui::Action const* action_text;
    };

    static constexpr ice::ecs::ArchetypeDefinition<ice::UIPage> Constant_Archetype_UIPage{ };
    static constexpr ice::ecs::ArchetypeDefinition<ice::UIElement, ice::UIButton> Constant_Archetype_UIButton{ };

    using Query_UIElements = ice::ecs::QueryDefinition<ice::UIElement const&, ice::UIButton const&>;

    static constexpr ice::ecs::ArchetypeInfo Constant_UIArchetypes[]{
        Constant_Archetype_UIPage,
        Constant_Archetype_UIButton
    };

    IceWorldTrait_GameUI::IceWorldTrait_GameUI(
        ice::Allocator& alloc
    ) noexcept
        : _allocator{ alloc }
        , _entity_tracker{ alloc }
        , _pages_info{ _allocator }
        , _pages{ _allocator }
    {
    }

    void IceWorldTrait_GameUI::on_activate(
        ice::Engine& engine,
        ice::EngineRunner& runner,
        ice::WorldPortal& portal
    ) noexcept
    {
        ice::vec2u const extent = runner.graphics_device().swapchain().extent();
        _size_fb = ice::vec2f(extent.x, extent.y);

        portal.storage().create_named_object<Query_UIElements::Query>("ice.query.ui-elements"_sid,
            portal.entity_storage().create_query(portal.allocator(), Query_UIElements{})
        );
    }

    void IceWorldTrait_GameUI::on_deactivate(
        ice::Engine& engine,
        ice::EngineRunner& runner,
        ice::WorldPortal& portal
    ) noexcept
    {
        portal.storage().destroy_named_object<Query_UIElements::Query>("ice.query.ui-elements"_sid);

        for (auto const& entry : _pages_info)
        {
            portal.allocator().deallocate(entry.value.draw_data.vertices.data());
            portal.allocator().deallocate(entry.value.draw_data.colors.data());
            portal.allocator().deallocate(entry.value.elements.data());
        }
    }

    void IceWorldTrait_GameUI::on_update(
        ice::EngineFrame& frame,
        ice::EngineRunner& runner,
        ice::WorldPortal& portal
    ) noexcept
    {
        static PageInfo invalid_page{ .data = nullptr };

        _entity_tracker.refresh_handles(runner.previous_frame().shards());

        ice::u64 const visible_page_hash = ice::hash(_visible_page);
        PageInfo const& visible_page_info = ice::pod::hash::get(
            _pages_info,
            visible_page_hash,
            invalid_page
        );

        ice::vec2i size;
        if (ice::shards::inspect_last(frame.shards(), ice::platform::Shard_WindowSizeChanged, size))
        {
            _size_fb = ice::vec2f(size.x, size.y);

            if (visible_page_info.data != nullptr)
            {
                portal.execute(
                    [this](ice::EngineFrame& frame, ice::EngineRunner& runner, PageInfo const& page_info) noexcept -> ice::Task<>
                    {
                        co_await update_ui(frame, runner, page_info);

                        ice::EngineFrame& last_frame = co_await runner.schedule_next_frame();

                        ice::shards::push_back(
                            last_frame.shards(),
                            ice::Shard_GameUI_Updated | page_info.name.data()
                        );
                    }(frame, runner, visible_page_info)
                );
            }
        }

        ice::input::InputEvent ievent[2];
        if (ice::shards::inspect_first(frame.shards(), ice::Shard_InputEventAxis, ievent))
        {
            _pos_mouse = ice::vec2f(ievent[0].value.axis.value_i32, ievent[1].value.axis.value_i32);
        }

        ice::pod::Array<ice::c8utf const*> page_names{ frame.allocator() };
        if (ice::shards::inspect_all<ice::c8utf const*>(runner.previous_frame().shards(), ice::Shard_GameUI_Load, page_names) > 0)
        {
            for (ice::c8utf const* page_name : page_names)
            {
                portal.execute(
                    load_ui(portal.allocator(), frame, runner, page_name)
                );
            }
        }

        ice::vec2f const pos_in_fb = {
            _pos_mouse.x,
            _pos_mouse.y
        };

        Query_UIElements::Query& query = *portal.storage().named_object<Query_UIElements::Query>("ice.query.ui-elements"_sid);
        ice::ecs::query::for_each_entity(
            query,
            [&, this](ice::UIElement const& element, UIButton const& button) noexcept
            {
                if (ice::pod::hash::has(_pages_info, element.page_hash))
                {
                    static PageInfo invalid_page{ .data = nullptr };
                    PageInfo const& page_info = ice::pod::hash::get(_pages_info, element.page_hash, invalid_page);

                    if (page_info.data != nullptr)
                    {
                        //if (button.action_text != nullptr)
                        //{
                        //    ice::ui::Action const& action = *button.action_on_click;
                        //    ice::ui::ShardInfo const shard_info = page_info.data->ui_shards[action.type_i];
                        //    ice::ShardID const shardid = ice::shard_id(shard_info.shard_name, ice::detail::Constant_ShardPayloadID<ice::c8utf const*>);

                        //    ice::Shard const shard = ice::shards::find_last_of(runner.previous_frame().shards(), ice::shard_create(shardid));

                        //    ice::c8utf const* text = nullptr;
                        //    if (ice::shard_inspect(shard, text))
                        //    {
                        //    }
                        //}

                        ice::ui::Element const& elem = page_info.elements[element.element_idx];
                        if (elem.hitbox.left <= pos_in_fb.x
                            && elem.hitbox.right >= pos_in_fb.x
                            && elem.hitbox.top <= pos_in_fb.y
                            && elem.hitbox.bottom >= pos_in_fb.y)
                        {
                            bool left_click = false;
                            ice::shards::inspect_each<ice::input::InputEvent>(
                                frame.shards(),
                                ice::Shard_InputEventButton,
                                [&, this](ice::input::InputEvent const& iev)
                                {
                                    auto constexpr mouse_left_button = ice::input::input_identifier(
                                        ice::input::DeviceType::Mouse,
                                        ice::input::MouseInput::ButtonLeft
                                    );

                                    left_click |= (iev.identifier == mouse_left_button) && iev.value.button.state.clicked;
                                }
                            );

                            if (left_click && button.action_on_click != nullptr)
                            {
                                ice::ui::Action const& action = *button.action_on_click;

                                ice::ui::ShardInfo const shard_info = page_info.data->ui_shards[action.type_i];
                                ice::ShardID const shardid = ice::shard_id(shard_info.shard_name, ice::detail::Constant_ShardPayloadID<ice::ecs::Entity>);

                                ice::shards::push_back(frame.shards(), ice::shard_create(shardid) | ice::ecs::Entity{});
                                ICE_LOG(ice::LogSeverity::Debug, ice::LogTag::Engine, "Clicked!");
                            }
                        }
                    }
                }
            }
        );

        ice::shards::inspect_each<ice::c8utf const*>(
            runner.previous_frame().shards(),
            ice::Shard_GameUI_Show,
            [&, this](ice::c8utf const* page_name)
            {
                ice::u64 const page_hash = ice::hash(page_name);

                if (ice::pod::hash::has(_pages_info, page_hash))
                {
                    PageInfo const& page_info = ice::pod::hash::get(_pages_info, page_hash, invalid_page);

                    if (page_info.data != nullptr)
                    {
                        ice::RenderUIRequest* request = frame.create_named_object<ice::RenderUIRequest>(ice::stringid(page_name));
                        request->id = page_hash;
                        request->position = ice::vec2f{ page_info.data->positions[0].x, page_info.data->positions[0].y };
                        request->draw_data = &page_info.draw_data;
                        request->update_only = false;

                        ice::shards::push_back(frame.shards(), ice::Shard_RenderUIData | (ice::RenderUIRequest const*) request);
                        _visible_page = page_name;
                    }
                }

                page_name = nullptr;
            }
        );

        ice::shards::inspect_each<ice::c8utf const*>(
            runner.previous_frame().shards(),
            ice::Shard_GameUI_Updated,
            [&, this](ice::c8utf const* page_name)
            {
                ice::u64 const page_hash = ice::hash(page_name);

                if (ice::pod::hash::has(_pages_info, page_hash))
                {
                    PageInfo const& page_info = ice::pod::hash::get(_pages_info, page_hash, invalid_page);

                    if (page_info.data != nullptr && _visible_page == page_name)
                    {
                        ice::RenderUIRequest* request = frame.create_named_object<ice::RenderUIRequest>(ice::stringid(page_name));
                        request->id = page_hash;
                        request->position = ice::vec2f{ page_info.data->positions[0].x, page_info.data->positions[0].y };
                        request->draw_data = &page_info.draw_data;
                        request->update_only = true;

                        ice::shards::push_back(frame.shards(), ice::Shard_RenderUIData | (ice::RenderUIRequest const*)request);
                    }
                }

                page_name = nullptr;
            }
        );

        {
            if (visible_page_info.data != nullptr)
            {
                ice::ui::UIData const* const uidata = visible_page_info.data;

                ice::u32 idx = 0;
                for (ice::ui::ElementInfo const& element : uidata->elements)
                {
                    ice::ui::Element const& element_layout = visible_page_info.elements[idx];

                    if (element.type == ui::ElementType::Button)
                    {
                        ice::ui::ButtonInfo const& button_info = uidata->data_buttons[element.type_data_i];

                        ice::DrawTextCommand* draw_text = frame.create_named_object<ice::DrawTextCommand>(
                            ice::StringID{ ice::StringID_Hash{ visible_page_hash + element.type_data_i } }
                        );

                        draw_text->font = u8"calibri";
                        draw_text->font_size = uidata->fonts[0].size;
                        draw_text->text = ice::Utf8String{
                            reinterpret_cast<ice::c8utf const*>(
                                ice::memory::ptr_add(uidata->additional_data, button_info.text_offset)
                            ),
                            button_info.text_size
                        };

                        //ice::ui::Position page_pos = uidata->positions[0];
                        ice::ui::Position pos = ice::ui::rect_position(element_layout.contentbox); // uidata->positions[element.pos_i];
                        //pos.x += entry.value->uniform.position.x;
                        //pos.y += entry.value->uniform.position.y;

                        ice::ui::Size size = ice::ui::rect_size(element_layout.contentbox); // uidata->sizes[element.size_i];
                        draw_text->position = ice::vec2u{
                            (ice::u32)(pos.x),
                            (ice::u32)(pos.y + size.height)
                        };

                        ice::shards::push_back(
                            frame.shards(),
                            ice::Shard_DrawTextCommand | (ice::DrawTextCommand const*)draw_text
                        );
                    }

                    idx += 1;
                }
            }
        }
    }

    auto IceWorldTrait_GameUI::load_ui(
        ice::Allocator& alloc,
        ice::EngineFrame& frame,
        ice::EngineRunner& runner,
        ice::Utf8String name
    ) noexcept -> ice::Task<>
    {
        ice::u64 const page_hash = ice::hash(name);
        if (ice::pod::hash::has(_pages_info, page_hash))
        {
            co_return;
        }

        ice::pod::hash::set(_pages_info, page_hash, PageInfo{ .data = nullptr });

        ice::Asset page_asset = co_await runner.asset_storage().request(
            ice::ui::AssetType_UIPage,
            name,
            AssetState::Loaded
        );

        if (ice::asset_check(page_asset, AssetState::Loaded) == false)
        {
            ICE_LOG(
                ice::LogSeverity::Warning, ice::LogTag::Game,
                "UI Page with name {} couldn't be loaded.",
                ice::String{ (char const*)name.data(), name.size() }
            );
            co_return;
        }

        co_await runner.schedule_next_frame();

        using ice::ui::ElementType;

        ice::ui::UIData const* const page_data = reinterpret_cast<ice::ui::UIData const*>(page_asset.data.location);

        ice::u32 const count_elements = ice::size(page_data->elements);
        ice::u32 count_interactive_element = 0;

        {
            for (ice::ui::ElementInfo const& element_info : page_data->elements)
            {
                if (element_info.type == ElementType::Button)
                {
                    count_interactive_element += 1;
                }
            }

        }

        //ice::pod::Array<ice::UIElement> elements{ alloc };
        //ice::pod::array::resize(elements, count_interactive_element);

        ice::pod::Array<ice::ecs::Entity> element_entities{ alloc };
        ice::pod::array::resize(element_entities, count_interactive_element);

        ice::ecs::Entity const page_entity = runner.entity_index().create();

        bool const ui_entities_created = runner.entity_index().create_many(element_entities);
        ICE_LOG_IF(
            ui_entities_created == false,
            ice::LogSeverity::Error, ice::LogTag::Engine,
            "Failed to create {} UI entities!",
            count_interactive_element
        );

        ice::Span<ice::UIElement> out_elements;
        ice::Span<ice::UIButton> out_buttons;

        if (ui_entities_created && count_interactive_element > 0)
        {
            ice::ecs::queue_set_archetype_with_data(
                runner.current_frame().entity_operations(),
                element_entities,
                ice::Constant_Archetype_UIButton,
                out_elements,
                out_buttons
            );

            ice::u32 idx_button = 0;
            ice::u8 idx_element = 0;

            for (ice::ui::ElementInfo const& element_data : page_data->elements)
            {
                ice::UIElement& element = out_elements[idx_button];

                if (element_data.type == ElementType::Button)
                {
                    out_buttons[idx_button].action_on_click = nullptr;
                    out_buttons[idx_button].action_text = nullptr;

                    using ice::ui::ActionType;
                    using ice::ui::ActionData;
                    using ice::ui::Property;

                    ice::ui::ButtonInfo const& button_info = page_data->data_buttons[element_data.type_data_i];
                    if (button_info.action_on_click_i != ice::u16{ 0xffff })
                    {
                        ice::ui::Action const& action_on_click = page_data->ui_actions[button_info.action_on_click_i];
                        out_buttons[idx_button].action_on_click = &action_on_click;
                    }
                    if (button_info.action_text_i != ice::u16{ 0xffff })
                    {
                        ice::ui::Action const& action_text = page_data->ui_actions[button_info.action_text_i];
                        out_buttons[idx_button].action_text = &action_text;
                    }

                    //element.page_entity = page_entity;
                    element.page_hash = page_hash;
                    element.element_idx = idx_element;
                    idx_button += 1;
                }

                idx_element += 1;
            }

        }

        {
            ice::EngineFrame& new_frame = co_await runner.schedule_next_frame();

            _entity_tracker.track_entity(page_entity, ice::stringid(name));

            void* element_layout_data = alloc.allocate(sizeof(ice::ui::Element) * count_elements);
            ice::ui::Element* element_layouts = reinterpret_cast<ice::ui::Element*>(element_layout_data);
            ice::memset(element_layouts, 0, sizeof(ice::ui::Element) * count_elements);

            ice::u32 size_vertice_data = 0;
            ice::u32 size_vertice_color = 0;

            for (ice::u32 idx = 0; idx < count_elements; ++idx)
            {
                ice::ui::ElementInfo const& element_info = page_data->elements[idx];

                if (element_info.type == ui::ElementType::Button)
                {
                    size_vertice_data += sizeof(ice::vec2f) * (Constant_DebugBoundingBox + 4 + Constant_DebugContentBox);
                    size_vertice_color += sizeof(ice::vec4f) * (Constant_DebugBoundingBox + 4 + Constant_DebugContentBox);
                }
            }

            ice::vec2f* vertice_data = reinterpret_cast<ice::vec2f*>(alloc.allocate(size_vertice_data));
            ice::vec4f* vertice_color = reinterpret_cast<ice::vec4f*>(alloc.allocate(size_vertice_color));
            ice::vec2f* vertice_data_it = vertice_data;
            ice::vec4f* vertice_color_it = vertice_color;

            // Calculate explicit sizes
            for (ice::u32 idx = 0; idx < count_elements; ++idx)
            {
                ice::ui::ElementInfo const& current_element_data = page_data->elements[idx];
                ice::ui::Element& current_element = element_layouts[idx];
                ice::ui::Element& parent_element = element_layouts[current_element_data.parent];

                if (current_element_data.type == ui::ElementType::Button)
                {
                    current_element.draw_data.vertice_count = (Constant_DebugBoundingBox + 4 + Constant_DebugContentBox);
                    current_element.draw_data.vertices = { vertice_data_it, current_element.draw_data.vertice_count };
                    current_element.draw_data.colors = { vertice_color_it, current_element.draw_data.vertice_count };

                    vertice_data_it += current_element.draw_data.vertice_count;
                    vertice_color_it += current_element.draw_data.vertice_count;
                }

                if (parent_element.child == nullptr)
                {
                    if (&parent_element != &current_element)
                    {
                        parent_element.child = &current_element;
                    }
                }
                else
                {
                    ice::ui::Element* sibling = parent_element.child;
                    while (sibling->sibling != nullptr)
                    {
                        sibling = sibling->sibling;
                    }
                    sibling->sibling = &current_element;
                }

                current_element.definition = &current_element_data;
            }

            PageInfo const page_info{
                .name = name,
                .data = page_data,
                .elements = { element_layouts, count_elements },
                .draw_data = {
                    .vertices = { vertice_data, (size_vertice_data / sizeof(ice::vec2f)) },
                    .colors = { vertice_color, (size_vertice_data / sizeof(ice::vec2f)) },
                    .vertice_count = { (size_vertice_data / sizeof(ice::vec2f)) },
                },
                .asset_handle = page_asset.handle,
            };

            co_await update_ui(new_frame, runner, page_info);

            {
                ice::EngineFrame& last_frame = co_await runner.schedule_next_frame();

                ice::shards::push_back(
                    last_frame.shards(),
                    ice::Shard_GameUI_Loaded | name.data()
                );

                ice::pod::hash::set(
                    _pages_info,
                    page_hash,
                    page_info
                );

                ice::ecs::queue_set_archetype_with_data(
                    runner.current_frame().entity_operations(),
                    page_entity,
                    ice::Constant_Archetype_UIPage,
                    ice::UIPage{ .page_hash = page_hash }
                );
            }
        }
    }

    auto IceWorldTrait_GameUI::update_ui(
        ice::EngineFrame& frame,
        ice::EngineRunner& runner,
        PageInfo const& page_info
    ) noexcept -> ice::Task<>
    {
        using ice::ui::UpdateStage;
        using ice::ui::UpdateResult;
        using ice::ui::ElementFlags;

        ice::u32 const count_elements = ice::size(page_info.data->elements);

        ice::pod::Array<ice::ui::UpdateResult> results{ frame.allocator() };
        ice::pod::array::resize(results, count_elements);

        co_await runner.thread_pool();

        auto const contains_unresolved = [&]() noexcept
        {
            for (UpdateResult result : results)
            {
                if (result == UpdateResult::Unresolved)
                {
                    return true;
                }
            }
            return false;
        };

        for (ice::u32 idx = 0; idx < count_elements; ++idx)
        {
            ice::ui::ElementInfo const& current_element_data = page_info.data->elements[idx];
            ice::ui::Element& current_element = page_info.elements[idx];
            ice::ui::Element& parent_element = page_info.elements[current_element_data.parent];

            results[idx] = ice::ui::element_update(
                UpdateStage::ExplicitSize,
                *page_info.data,
                parent_element,
                current_element_data,
                current_element
            );
        }

        // Calculate auto sizes (childs to parents)
        if (contains_unresolved())
        {
            for (ice::i32 idx = count_elements - 1; idx >= 0; --idx)
            {
                ice::ui::ElementInfo const& current_element_data = page_info.data->elements[idx];
                ice::ui::Element& current_element = page_info.elements[idx];
                ice::ui::Element& parent_element = page_info.elements[current_element_data.parent];

                if (results[idx] != UpdateResult::Resolved)
                {
                    results[idx] = ice::ui::element_update(
                        UpdateStage::AutoSize,
                        *page_info.data,
                        parent_element,
                        current_element_data,
                        current_element
                    );
                }
            }
        }

        // Calculate stretch sizes (childs to siblings)
        if (contains_unresolved())
        {
            for (ice::i32 idx = count_elements - 1; idx >= 0; --idx)
            {
                if (results[idx] == UpdateResult::Resolved)
                {
                    continue;
                }

                ice::ui::ElementInfo const& current_element_data = page_info.data->elements[idx];
                ice::ui::Element& current_element = page_info.elements[idx];
                ice::ui::Element& parent_element = page_info.elements[current_element_data.parent];

                results[idx] = ice::ui::element_update(
                    UpdateStage::StretchSize,
                    *page_info.data,
                    parent_element,
                    current_element_data,
                    current_element
                );
            }
        }

        // Calculate positions and some remaining sizes
        for (ice::u32 idx = 0; idx < count_elements; ++idx)
        {
            ice::ui::ElementInfo const& current_element_data = page_info.data->elements[idx];
            ice::ui::Element& current_element = page_info.elements[idx];
            ice::ui::Element& parent_element = page_info.elements[current_element_data.parent];

            ice::ui::element_update(
                UpdateStage::Position,
                *page_info.data,
                parent_element,
                current_element_data,
                current_element
            );
        }

        ICE_ASSERT(
            contains_unresolved() == false,
            "UI sizes should be fully resolved!"
        );

        for (ice::ui::Element const& element : page_info.elements)
        {
            if (element.definition->type == ui::ElementType::Button)
            {
                ice::vec2f* vertices = element.draw_data.vertices.data();
                ice::vec4f* colors = element.draw_data.colors.data();

                if constexpr (Constant_DebugBoundingBox == 4)
                {
                    colors[0 + 0] = colors[1 + 0] = colors[2 + 0] = colors[3 + 0] = ice::vec4f{ 0.8f, 0.8f, 0.8f, 0.2f };
                }

                colors[0 + Constant_DebugBoundingBox] =
                    colors[1 + Constant_DebugBoundingBox] =
                    colors[2 + Constant_DebugBoundingBox] =
                    colors[3 + Constant_DebugBoundingBox] =
                    ice::vec4f{ 0.2f, 0.6f, 0.8f, 0.7f };

                if constexpr (Constant_DebugContentBox == 4)
                {
                    colors[0 + Constant_DebugBoundingBox + Constant_DebugContentBox] =
                        colors[1 + Constant_DebugBoundingBox + Constant_DebugContentBox] =
                        colors[2 + Constant_DebugBoundingBox + Constant_DebugContentBox] =
                        colors[3 + Constant_DebugBoundingBox + Constant_DebugContentBox] =
                        ice::vec4f{ 0.9f, 0.2f, 0.2f, 0.3f };
                }

                ice::ui::Position const bpos = ice::ui::rect_position(element.bbox);
                ice::ui::Size const bsize = ice::ui::rect_size(element.bbox);

                ice::ui::Position const pos = ice::ui::rect_position(element.hitbox);
                ice::ui::Size const size = ice::ui::rect_size(element.hitbox);

                ice::ui::Position const cpos = ice::ui::rect_position(element.contentbox);
                ice::ui::Size const csize = ice::ui::rect_size(element.contentbox);

                // [0   2]
                // [1   3]

                if constexpr (Constant_DebugBoundingBox == 4)
                {
                    vertices[0 + 0].x = bpos.x;
                    vertices[0 + 0].y = bpos.y;
                    vertices[1 + 0].x = bpos.x;
                    vertices[1 + 0].y = bpos.y + bsize.height;
                    vertices[2 + 0].x = bpos.x + bsize.width;
                    vertices[2 + 0].y = bpos.y;
                    vertices[3 + 0].x = bpos.x + bsize.width;
                    vertices[3 + 0].y = bpos.y + bsize.height;
                }

                // [0   2]
                // [1   3]
                vertices[0 + Constant_DebugBoundingBox].x = pos.x;
                vertices[0 + Constant_DebugBoundingBox].y = pos.y;
                vertices[1 + Constant_DebugBoundingBox].x = pos.x;
                vertices[1 + Constant_DebugBoundingBox].y = pos.y + size.height;
                vertices[2 + Constant_DebugBoundingBox].x = pos.x + size.width;
                vertices[2 + Constant_DebugBoundingBox].y = pos.y;
                vertices[3 + Constant_DebugBoundingBox].x = pos.x + size.width;
                vertices[3 + Constant_DebugBoundingBox].y = pos.y + size.height;

                // [0   2]
                // [1   3]

                if constexpr (Constant_DebugContentBox == 4)
                {
                    vertices[0 + Constant_DebugBoundingBox + Constant_DebugContentBox].x = cpos.x;
                    vertices[0 + Constant_DebugBoundingBox + Constant_DebugContentBox].y = cpos.y;
                    vertices[1 + Constant_DebugBoundingBox + Constant_DebugContentBox].x = cpos.x;
                    vertices[1 + Constant_DebugBoundingBox + Constant_DebugContentBox].y = cpos.y + csize.height;
                    vertices[2 + Constant_DebugBoundingBox + Constant_DebugContentBox].x = cpos.x + csize.width;
                    vertices[2 + Constant_DebugBoundingBox + Constant_DebugContentBox].y = cpos.y;
                    vertices[3 + Constant_DebugBoundingBox + Constant_DebugContentBox].x = cpos.x + csize.width;
                    vertices[3 + Constant_DebugBoundingBox + Constant_DebugContentBox].y = cpos.y + csize.height;
                }
            }
        }
    }

    void register_trait_gameui(ice::WorldTraitArchive& trait_archive) noexcept
    {
        trait_archive.register_trait(
            ice::Constant_TraitName_GameUI,
            ice::WorldTraitDescription
            {
                .factory = ice::detail::generic_trait_factory<ice::IceWorldTrait_GameUI>,
                .defined_archetypes = Constant_UIArchetypes
            }
        );
    }

} // namespace ice
