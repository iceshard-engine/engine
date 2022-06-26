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
#include <ice/platform_event.hxx>
#include <ice/stack_string.hxx>
#include <ice/shard_container.hxx>
#include <ice/task_scheduler.hxx>
#include <ice/task_thread_pool.hxx>
#include <ice/pod/array.hxx>
#include <ice/asset_storage.hxx>

namespace ice
{

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

        ice::ShardName onclick_shard_name;
    };

    static constexpr ice::ecs::ArchetypeDefinition<ice::UIPage> Constant_Archetype_UIPage{ };
    static constexpr ice::ecs::ArchetypeDefinition<ice::UIElement, ice::UIButton> Constant_Archetype_UIButton{ };

    using Query_UIElements = ice::ecs::QueryDefinition<ice::UIElement const&>;

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
            portal.allocator().deallocate(entry.value.elements.data());
        }
    }

    void IceWorldTrait_GameUI::on_update(
        ice::EngineFrame& frame,
        ice::EngineRunner& runner,
        ice::WorldPortal& portal
    ) noexcept
    {
        _entity_tracker.refresh_handles(runner.previous_frame().shards());

        ice::vec2i size;
        if (ice::shards::inspect_last(frame.shards(), ice::platform::Shard_WindowSizeChanged, size))
        {
            _size_fb = ice::vec2f(size.x, size.y);
        }

        ice::input::InputEvent ievent[2];
        if (ice::shards::inspect_first(frame.shards(), ice::Shard_InputEventAxis, ievent))
        {
            _pos_mouse = ice::vec2f(ievent[0].value.axis.value_i32, ievent[1].value.axis.value_i32);
            //ICE_LOG(ice::LogSeverity::Debug, ice::LogTag::Engine, "({}, {})", _pos_mouse.x, _pos_mouse.y);
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
            _pos_mouse.x/* / (_size_fb.y * 0.5f) - 1.f*/,
            _pos_mouse.y/* / (_size_fb.y * 0.5f) - 1.f*/\
        };

        Query_UIElements::Query& query = *portal.storage().named_object<Query_UIElements::Query>("ice.query.ui-elements"_sid);
        ice::ecs::query::for_each_entity(
            query,
            [&, this](ice::UIElement const& element) noexcept
            {
                if (ice::pod::hash::has(_pages_info, element.page_hash))
                {
                    static PageInfo invalid_page{ .data = nullptr };
                    PageInfo const& page_info = ice::pod::hash::get(_pages_info, element.page_hash, invalid_page);

                    ice::ui::Element const& elem = page_info.elements[element.element_idx];
                    if (elem.hitbox.left <= pos_in_fb.x
                        && elem.hitbox.right >= pos_in_fb.x
                        && elem.hitbox.top <= pos_in_fb.y
                        && elem.hitbox.bottom >= pos_in_fb.y)
                    {
                        ICE_LOG(ice::LogSeverity::Debug, ice::LogTag::Engine, "INSIDE!");
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
                    static PageInfo invalid_page{ .data = nullptr };
                    PageInfo const& page_info = ice::pod::hash::get(_pages_info, page_hash, invalid_page);

                    if (page_info.data != nullptr)
                    {
                        ice::RenderUIRequest* request = frame.create_named_object<ice::RenderUIRequest>(ice::stringid(page_name));
                        request->id = page_hash;
                        request->position = ice::vec2f{ page_info.data->positions[0].x, page_info.data->positions[0].y };
                        request->data = page_info.data;
                        request->data_layouts = page_info.elements.data();

                        ice::shards::push_back(frame.shards(), ice::Shard_RenderUIData | (ice::RenderUIRequest const*) request);
                        _visible_page = page_name;
                    }
                }

                page_name = nullptr;
            }
        );
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

        if (ui_entities_created && count_interactive_element > 0)
        {
            static ice::ecs::ArchetypeDefinition<ice::UIElement, ice::UIButton> constexpr TempArchetype;

            constexpr ice::StaticArray<ice::u32, 2> const idx_map = ice::ecs::detail::argument_idx_map<ice::UIElement, ice::UIButton>(
                TempArchetype.component_identifiers
            );

            constexpr ice::ecs::EntityOperations::ComponentInfo const temp_component_info{
                .names = ice::make_span(TempArchetype.component_identifiers).subspan(1),
                .sizes = ice::make_span(TempArchetype.component_sizes).subspan(1),
                .offsets = ice::make_span(TempArchetype.component_alignments).subspan(1)
            };

            ice::UIElement* ui_elements = nullptr;
            ice::UIButton* ui_buttons = nullptr;

            {
                ice::u32 const entity_count = ice::size(element_entities);
                ice::usize additional_data_size = sizeof(ice::ecs::EntityHandle) * entity_count;

                // Data for storing component info
                additional_data_size += sizeof(ice::ecs::EntityOperations::ComponentInfo);
                additional_data_size += temp_component_info.names.size_bytes();
                additional_data_size += temp_component_info.sizes.size_bytes();
                additional_data_size += temp_component_info.offsets.size_bytes();
                additional_data_size += alignof(ice::UIElement) + sizeof(ice::UIElement) * count_interactive_element;
                additional_data_size += alignof(ice::UIButton) + sizeof(ice::UIButton) * count_interactive_element;

                void* operation_data = nullptr;
                ice::ecs::EntityOperation* operation = runner.current_frame().entity_operations().new_storage_operation(
                    additional_data_size,
                    operation_data
                );

                // Set entity handles
                ice::ecs::EntityHandle* entities_ptr = reinterpret_cast<ice::ecs::EntityHandle*>(operation_data);
                operation_data = entities_ptr + entity_count;

                for (ice::u32 idx = 0; idx < entity_count; ++idx)
                {
                    entities_ptr[idx] = detail::make_empty_entity_handle(element_entities[idx]);
                }

                // Set component info object
                ice::StringID* names_ptr = reinterpret_cast<ice::StringID*>(operation_data);
                ice::memcpy(names_ptr, temp_component_info.names.data(), temp_component_info.names.size_bytes());
                operation_data = ice::memory::ptr_add(operation_data, temp_component_info.names.size_bytes());

                ice::u32* sizes_ptr = reinterpret_cast<ice::u32*>(operation_data);
                ice::memcpy(sizes_ptr, temp_component_info.sizes.data(), temp_component_info.sizes.size_bytes());
                operation_data = ice::memory::ptr_add(operation_data, temp_component_info.sizes.size_bytes());

                ice::u32* offsets_ptr = reinterpret_cast<ice::u32*>(operation_data);
                ice::memcpy(offsets_ptr, temp_component_info.offsets.data(), temp_component_info.offsets.size_bytes());
                operation_data = ice::memory::ptr_add(operation_data, temp_component_info.offsets.size_bytes());

                ice::ecs::EntityOperations::ComponentInfo* component_info_ptr = reinterpret_cast<ice::ecs::EntityOperations::ComponentInfo*>(
                    operation_data
                );
                operation_data = component_info_ptr + 1;

                ice::u32 const component_count = ice::size(temp_component_info.names);
                component_info_ptr->names = ice::Span<ice::StringID const>{ names_ptr, component_count };
                component_info_ptr->sizes = ice::Span<ice::u32 const>{ sizes_ptr, component_count };
                component_info_ptr->offsets = ice::Span<ice::u32 const>{ offsets_ptr, component_count };

                // Copy over component data
                void const* const data_beg = operation_data;

                ui_elements = reinterpret_cast<ice::UIElement*>(
                    ice::memory::ptr_align_forward(operation_data, alignof(ice::UIElement))
                );
                ui_buttons = reinterpret_cast<ice::UIButton*>(
                    ice::memory::ptr_align_forward(ui_elements + component_count, alignof(ice::UIButton))
                );

                // Save the offset
                offsets_ptr[idx_map[0] - 1] = ice::memory::ptr_distance(data_beg, ui_elements);
                offsets_ptr[idx_map[1] - 1] = ice::memory::ptr_distance(data_beg, ui_buttons);

                operation->archetype = TempArchetype;
                operation->entities = entities_ptr;
                operation->entity_count = entity_count;
                operation->notify_entity_changes = false;
                operation->component_data = component_info_ptr;
                operation->component_data_size = ice::memory::ptr_distance(component_info_ptr, ui_buttons + component_count);
            }

            ice::u32 idx = 0;
            ice::u8 idx_element = 0;

            for (ice::ui::ElementInfo const& element_data : page_data->elements)
            {
                ice::UIElement& element = ui_elements[idx];

                if (element_data.type == ElementType::Button)
                {
                    //element.page_entity = page_entity;
                    element.page_hash = page_hash;
                    element.element_idx = idx_element;
                    idx += 1;
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

            using ice::ui::UpdateStage;
            using ice::ui::UpdateResult;
            using ice::ui::ElementFlags;

            ice::pod::Array<ice::ui::UpdateResult> results{ new_frame.allocator() };
            ice::pod::array::resize(results, count_elements);

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

            // Calculate explicit sizes
            for (ice::u32 idx = 0; idx < count_elements; ++idx)
            {
                ice::ui::ElementInfo const& current_element_data = page_data->elements[idx];
                ice::ui::Element& current_element = element_layouts[idx];
                ice::ui::Element& parent_element = element_layouts[current_element_data.parent];

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

                results[idx] = ice::ui::element_update(
                    UpdateStage::ExplicitSize,
                    *page_data,
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
                    ice::ui::ElementInfo const& current_element_data = page_data->elements[idx];
                    ice::ui::Element& current_element = element_layouts[idx];
                    ice::ui::Element& parent_element = element_layouts[current_element_data.parent];

                    if (results[idx] != UpdateResult::Resolved)
                    {
                        results[idx] = ice::ui::element_update(
                            UpdateStage::AutoSize,
                            *page_data,
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

                    ice::ui::ElementInfo const& current_element_data = page_data->elements[idx];
                    ice::ui::Element& current_element = element_layouts[idx];
                    ice::ui::Element& parent_element = element_layouts[current_element_data.parent];

                    results[idx] = ice::ui::element_update(
                        UpdateStage::StretchSize,
                        *page_data,
                        parent_element,
                        current_element_data,
                        current_element
                    );
                }
            }

            // Calculate positions and some remaining sizes
            for (ice::u32 idx = 0; idx < count_elements; ++idx)
            {
                ice::ui::ElementInfo const& current_element_data = page_data->elements[idx];

                ice::ui::element_update(
                    UpdateStage::Position,
                    *page_data,
                    element_layouts[current_element_data.parent],
                    current_element_data,
                    element_layouts[idx]
                );
            }

            ICE_ASSERT(
                contains_unresolved() == false,
                "UI sizes should be fully resolved!"
            );

            PageInfo const page_info{
                .name = name,
                .data = page_data,
                .elements = { element_layouts, count_elements },
                .asset_handle = page_asset.handle,
            };

            ice::shards::push_back(
                new_frame.shards(),
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
