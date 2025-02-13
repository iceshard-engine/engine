#include "iceshard_world_devui.hxx"
#include <ice/container/hashmap.hxx>
#include <ice/world/world_trait.hxx>
#include <ice/devui_imgui.hxx>

namespace ice
{

    namespace detail
    {

        void devui_handlers_table(
            ice::Span<ice::IceshardEventHandler const> handlers,
            ice::Span<ice::UniquePtr<ice::IceshardTraitContext> const> traits
        ) noexcept;

    } // namespace detail

    auto IceshardWorld::create_devui(
        ice::Allocator& alloc,
        ice::IceshardWorldContext& context
    ) noexcept -> ice::UniquePtr<IceshardWorld::DevUI>
    {
        if (ice::devui_available())
        {
            return ice::make_unique<DevUI>(alloc, alloc, *this, context);
        }
        return {};
    }

    IceshardWorld::DevUI::DevUI(
        ice::Allocator& alloc,
        ice::IceshardWorld& world,
        ice::IceshardWorldContext& context
    ) noexcept
        : DevUIWidget{ {.category = "Engine/Worlds", .name=ice::stringid_hint(world.worldID)} }
        , _allocator{ alloc }
        , _world{ world }
        , _context{ context }
    {
        ice::devui_register_widget(this);
    }

    void IceshardWorld::DevUI::build_content() noexcept
    {
        ImVec2 const size = ImGui::GetContentRegionAvail();

        ImGui::PushItemWidth(size.x * 0.48f);
        ImGui::TextT("{}", _world.worldID);
        ImGui::PopItemWidth();

        if (ImGui::CollapsingHeader("Traits"))
        {
            for (ice::UniquePtr<IceshardTraitContext> const& trait : _world._traits)
            {
                TraitDevUI* trait_devui;
                if (trait->query_interface(trait_devui))
                {
                    if (ImGui::TreeNodeEx(trait_devui, ImGuiTreeNodeFlags_SpanAvailWidth, "%s", trait_devui->trait_name()._data))
                    {
                        trait_devui->build_content();
                        ImGui::TreePop();
                    }

                    // // if (ImGui::CollapsingHeader(trait_devui->trait_name()._data))
                    // {
                    //     if (ImGui::BeginChild("##stats", {}, ImGuiChildFlags_AutoResizeY | ImGuiChildFlags_Border))
                    //     {
                    //         // ImGui::Separator();
                    //     }
                    //     ImGui::EndChild();
                    // }
                }
            }
        }

        if (ImGui::CollapsingHeader("Handlers"))
        {
            IceshardWorldContext const& ctx = _context;

            static auto make_handler_list = [this](ice::String handler_type, auto const& hashmap) noexcept
                {
                    ImGui::TextT("{} handlers (count: {})", handler_type, ice::hashmap::count(hashmap));
                    if (ice::hashmap::any(hashmap))
                    {
                        detail::devui_handlers_table(ice::hashmap::values(hashmap), _world._traits);
                    }
                    else
                    {
                        ImGui::Separator();
                    }
                };

            make_handler_list("Logic", ctx._frame_handlers[0]);
            make_handler_list("Graphics", ctx._frame_handlers[1]);
            make_handler_list("Render", ctx._frame_handlers[2]);

            make_handler_list("Runner", ctx._runner_handlers);
        }
    }

    void detail::devui_handlers_table(
        ice::Span<ice::IceshardEventHandler const> handlers,
        ice::Span<ice::UniquePtr<ice::IceshardTraitContext> const> traits
    ) noexcept
    {
        if (ImGui::BeginTable("##frame-handler-list", 3, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg))
        {
            ImGui::TableSetupColumn("Event");
            ImGui::TableSetupColumn("Payload");
            ImGui::TableSetupColumn("Trait");
            ImGui::TableHeadersRow();

            for (IceshardEventHandler const& handler : handlers)
            {
                ImGui::TableNextRow();

                ice::UniquePtr<IceshardTraitContext> const& trait = traits[handler.trait_idx];

                ImGui::TableNextColumn();
                ImGui::TextT("{:x}", handler.event_id.name.value);

                ImGui::TableNextColumn();
                ImGui::TextT("{:x}", handler.event_id.payload.value);

                ice::TraitDevUI* trait_devui;
                if (trait->query_interface(trait_devui))
                {
                    ImGui::TableNextColumn();
                    ImGui::TextT("{}", trait_devui->trait_name());
                }
            }
            ImGui::EndTable();
        }
    }

} // namespace ice
