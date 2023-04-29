/// Copyright 2022 - 2022, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#include "render_ui_trait.hxx"
#include "game_ui_trait.hxx"
#include "game_ui_page.hxx"

#include <ice/game_ui.hxx>
#include <ice/game_render_traits.hxx>
#include <ice/ui_asset.hxx>
#include <ice/ui_element.hxx>
#include <ice/ui_element_info.hxx>
#include <ice/ui_button.hxx>
#include <ice/ui_label.hxx>
#include <ice/ui_action.hxx>
#include <ice/ui_shard.hxx>
#include <ice/ui_resource.hxx>
#include <ice/ui_font.hxx>
#include <ice/ui_data_utils.hxx>

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
#include <ice/string/static_string.hxx>
#include <ice/container/array.hxx>
#include <ice/shard_container.hxx>
#include <ice/asset_storage.hxx>
#include <ice/profiler.hxx>
#include <ice/task.hxx>

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

        ice::u64 page_hash;
        ice::u8 element_idx;
    };

    struct UIButton
    {
        static constexpr ice::StringID Identifier = "ice.component.ui-button"_sid;

        ice::ui::ActionInfo const* action_on_click;
    };

    static constexpr ice::ecs::ArchetypeDefinition<ice::UIPage> Constant_Archetype_UIPage{ };
    static constexpr ice::ecs::ArchetypeDefinition<ice::UIElement, ice::UIButton> Constant_Archetype_UIButton{ };

    using Query_UIElements = ice::ecs::QueryDefinition<ice::ecs::EntityHandle, ice::UIElement const&, ice::UIButton const&>;

    static constexpr ice::ecs::ArchetypeInfo Constant_UIArchetypes[]{
        Constant_Archetype_UIPage,
        Constant_Archetype_UIButton
    };

    IceWorldTrait_GameUI::IceWorldTrait_GameUI(
        ice::Allocator& alloc
    ) noexcept
        : _allocator{ alloc }
        , _entity_tracker{ alloc }
        , _pages{ _allocator }
    {
    }

    void IceWorldTrait_GameUI::on_activate(
        ice::Engine& engine,
        ice::EngineRunner& runner,
        ice::WorldPortal& portal
    ) noexcept
    {
        _swapchain_size = runner.graphics_device().swapchain().extent();

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

        for (GameUI_Page* page : _pages)
        {
            portal.allocator().destroy(page);
        }
    }

    void IceWorldTrait_GameUI::on_update(
        ice::EngineFrame& frame,
        ice::EngineRunner& runner,
        ice::WorldPortal& portal
    ) noexcept
    {
        ice::input::InputEvent ievent[2];
        if (ice::shards::inspect_first(frame.shards(), ice::Shard_InputEventAxis, ievent))
        {
            _pos_mouse = { ice::f32(ievent[0].value.axis.value_i32), ice::f32(ievent[1].value.axis.value_i32) };
        }

        for (ice::Shard shard : runner.previous_frame().shards())
        {
            if (shard == ice::platform::Shard_WindowResized)
            {
                ice::vec2i size;
                if (ice::shard_inspect(shard, size))
                {
                    _swapchain_size = ice::vec2u(size.x, size.y);
                    for (GameUI_Page* page : _pages)
                    {
                        page->resize(_swapchain_size);
                    }
                }
            }
            else if (shard == Shard_GameUI_Load)
            {
                char const* page_name;
                if (ice::shard_inspect(shard, page_name))
                {
                    portal.execute(load_ui(portal.allocator(), runner, page_name));
                }
            }
        }

        ice::vec2f const pos_in_fb = {
            _pos_mouse.x,
            _pos_mouse.y
        };

        Query_UIElements::Query& query = *portal.storage().named_object<Query_UIElements::Query>("ice.query.ui-elements"_sid);
        ice::ecs::query::for_each_entity(
            query,
            [&, this](ice::ecs::EntityHandle entity, ice::UIElement const& element, UIButton const& button) noexcept
            {
                using ice::ui::ElementState;

                GameUI_Page* const page = ice::hashmap::get(_pages, element.page_hash, nullptr);
                if (page != nullptr && page->visible())
                {
                    ice::ui::PageInfo const& page_info = page->info();
                    ice::ui::Element const& elem = page->element(element.element_idx);

                    if (elem.hitbox.left <= pos_in_fb.x
                        && elem.hitbox.right >= pos_in_fb.x
                        && elem.hitbox.top <= pos_in_fb.y
                        && elem.hitbox.bottom >= pos_in_fb.y)
                    {
                        page->set_element_state(elem, ElementState::Hoover);

                        bool left_click = false;
                        ice::shards::inspect_each<ice::input::InputEvent>(
                            frame.shards(),
                            ice::Shard_InputEventButton,
                            [&left_click](ice::input::InputEvent const& iev)
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
                            ice::ui::ActionInfo const& action = *button.action_on_click;
                            ice::ui::ShardInfo const shard_info = page_info.ui_shards[action.type_i];

                            ice::shards::push_back(frame.shards(), ice::shard(shard_info.shardid) | entity);
                            ICE_LOG(ice::LogSeverity::Debug, ice::LogTag::Engine, "Clicked!");
                        }
                    }
                    else
                    {
                        page->set_element_state(elem, ElementState::None);
                    }
                }
            }
        );

        ice::shards::inspect_each<char const*>(
            runner.previous_frame().shards(),
            ice::Shard_GameUI_Show,
            [&, this](char const* page_name)
            {
                ice::u64 const page_hash = ice::hash(page_name);
                ice::GameUI_Page* const page = ice::hashmap::get(_pages, page_hash, nullptr);
                if (page != nullptr)
                {
                    page->open(_swapchain_size);
                }
            }
        );

        ice::shards::inspect_each<char const*>(
            runner.previous_frame().shards(),
            ice::Shard_GameUI_Hide,
            [&, this](char const* page_name)
            {
                ice::u64 const page_hash = ice::hash(page_name);
                ice::GameUI_Page* const page = ice::hashmap::get(_pages, page_hash, nullptr);
                if (page != nullptr)
                {
                    page->close();
                }
            }
        );

        for (GameUI_Page* page : _pages)
        {
            if (page)
            {
                portal.execute(page->update(runner));
            }
        }
    }

    auto IceWorldTrait_GameUI::load_ui(
        ice::Allocator& alloc,
        ice::EngineRunner& runner,
        ice::String name
    ) noexcept -> ice::Task<>
    {
        ice::u64 const page_hash = ice::hash(name);
        if (ice::hashmap::has(_pages, page_hash))
        {
            co_return;
        }

        ice::Asset page_asset = runner.asset_storage().bind(
            ice::ui::AssetType_UIPage,
            name,
            AssetState::Loaded
        );
        page_asset.data = co_await runner.asset_storage().request(page_asset, AssetState::Loaded);
        //if (ice::asset_check(page_asset, AssetState::Loaded) == false)
        //{
        //    ICE_LOG(
        //        ice::LogSeverity::Warning, ice::LogTag::Game,
        //        "UI Page with name {} couldn't be loaded.",
        //        name
        //    );
        //    co_return;
        //}

        ice::GameUI_Page* page = nullptr;
        ice::hashmap::set(_pages, page_hash, page);
        page = alloc.create<ice::GameUI_Page>(alloc, page_asset, name);;

        ice::ui::PageInfo const& page_info = page->info();
        for (ice::ui::FontInfo const& font_info : page_info.fonts)
        {
            ice::String const font_name{
                reinterpret_cast<char const*>(ice::ptr_add(page_info.additional_data, { font_info.font_name_offset })),
                font_info.font_name_size
            };

            co_await runner.task_scheduler();
            auto font_asset = runner.asset_storage().bind(ice::AssetType_Font, font_name, AssetState::Loaded);
            co_await runner.asset_storage().request(font_asset, AssetState::Loaded);
        }

        co_await runner.schedule_current_frame();

        ice::EngineFrame& frame = runner.current_frame();

        using ice::ui::ElementType;

        // Find the number of interactive elements.
        ice::u32 count_interactive_element = 0;
        for (ice::ui::ElementInfo const& element_info : page_info.elements)
        {
            if (element_info.type == ElementType::Button)
            {
                count_interactive_element += 1;
            }
        }

        // Create components for interactive entities.
        if (count_interactive_element > 0)
        {
            ice::Array<ice::ecs::Entity> element_entities{ frame.allocator() };
            ice::array::resize(element_entities, count_interactive_element);

            // Create the entities
            bool const ui_entities_created = runner.entity_index().create_many(element_entities);
            ICE_LOG_IF(
                ui_entities_created == false,
                ice::LogSeverity::Error, ice::LogTag::Engine,
                "Failed to create {} UI entities!",
                count_interactive_element
            );

            if (ui_entities_created)
            {
                ice::Span<ice::UIElement> out_elements;
                ice::Span<ice::UIButton> out_buttons;

                ice::ecs::queue_set_archetype_with_data(
                    runner.current_frame().entity_operations(),
                    element_entities,
                    ice::Constant_Archetype_UIButton,
                    out_elements,
                    out_buttons
                );

                ice::u32 idx_button = 0;
                ice::u8 idx_element = 0;

                for (ice::ui::ElementInfo const& element_data : page_info.elements)
                {
                    ice::UIElement& element = out_elements[idx_button];

                    if (element_data.type == ElementType::Button)
                    {
                        out_buttons[idx_button].action_on_click = nullptr;

                        using ice::ui::ActionType;
                        using ice::ui::DataSource;

                        ice::ui::ButtonInfo const& button_info = page_info.data_buttons[element_data.type_data_i];
                        if (button_info.action_on_click_i != ice::u16{ 0xffff })
                        {
                            ice::ui::ActionInfo const& action_on_click = page_info.ui_actions[button_info.action_on_click_i];
                            out_buttons[idx_button].action_on_click = &action_on_click;
                        }

                        //element.page_entity = page_entity;
                        element.page_hash = page_hash;
                        element.element_idx = idx_element;
                        idx_button += 1;
                    }

                    idx_element += 1;
                }
            }
        }

        // Move to a separate font handler?
        for (ice::ui::FontInfo const& font_info : page_info.fonts)
        {
            ice::String const font_name{
                reinterpret_cast<char const*>(ice::ptr_add(page_info.additional_data, { font_info.font_name_offset })),
                font_info.font_name_size
            };

            ice::Asset font_asset = runner.asset_storage().bind(ice::AssetType_Font, font_name, AssetState::Loaded);
            font_asset.data = co_await runner.asset_storage().request(font_asset, AssetState::Loaded);
            if (ice::asset_check(font_asset, ice::AssetState::Loaded))
            {
                page->set_resource(font_info.resource_i, reinterpret_cast<ice::Font const*>(font_asset.data.location));
            }
        }

        ice::shards::push_back(
            frame.shards(),
            ice::Shard_GameUI_Loaded | ice::begin(name)
        );

        ice::hashmap::set(_pages, page_hash, page);
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
