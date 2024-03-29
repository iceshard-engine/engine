/// Copyright 2022 - 2023, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#include "imgui_system.hxx"

#include <ice/assert.hxx>
#include <imgui/imgui.h>
#undef assert

namespace ice::devui
{

    namespace detail
    {

        void create_alloc_tree_widget(ice::devui::ImGuiSystem& sys, ice::Allocator& tracked_alloc) noexcept
        {
            if constexpr (ice::Allocator::HasDebugInformation)
            {
                ice::AllocatorDebugInfo const* top_alloc = &tracked_alloc.debug_info();
                while (top_alloc->parent_allocator() != nullptr)
                {
                    top_alloc = top_alloc->parent_allocator();
                }

                sys.set_alloc_tree_widget(tracked_alloc.create<ImGui_AllocatorTreeWidget>(*top_alloc));
            }
        }

        auto create_imgui_trait(ice::Allocator& alloc, void* userdata) noexcept -> ice::UniquePtr<ice::Trait>
        {
            ice::UniquePtr<ice::devui::ImGuiTrait> trait = ice::make_unique<ice::devui::ImGuiTrait>(alloc, alloc);
            ice::devui::ImGuiSystem* system = reinterpret_cast<ImGuiSystem*>(userdata);
            system->set_trait(trait.get());
            return trait;
        }

        auto imgui_memalloc(size_t size, void* userdata) noexcept -> void*
        {
            return reinterpret_cast<ice::ProxyAllocator*>(userdata)->allocate(ice::usize{size}).memory;
        }

        auto imgui_memfree(void* ptr, void* userdata) noexcept -> void
        {
            return reinterpret_cast<ice::ProxyAllocator*>(userdata)->deallocate(ptr);
        }

    } // namespace detail

    ImGuiSystem::ImGuiSystem(ice::Allocator& alloc) noexcept
        : _allocator{ alloc, "ImGui" }
        , _render_trait{ nullptr }
        , _widget_alloc_tree{ nullptr }
        , _widgets{ alloc }
        , _inactive_widgets{ alloc }
    {
        ImGui::SetAllocatorFunctions(detail::imgui_memalloc, detail::imgui_memfree, &_allocator);
        ice::array::reserve(_widgets, 100);
        detail::create_alloc_tree_widget(*this, _allocator);
    }

    ImGuiSystem::~ImGuiSystem() noexcept
    {
        for (WidgetRuntimeInfo& info : _widgets)
        {
            _allocator.destroy(info.state);
        }

        _allocator.destroy(_widget_alloc_tree);
    }

    void ImGuiSystem::set_trait(ice::devui::ImGuiTrait* trait) noexcept
    {
        ICE_ASSERT(
            _render_trait == nullptr
            || trait == nullptr,
            "Setting ImGui trait failed!"
        );
        _render_trait = trait;

        if (_render_trait != nullptr)
        {
            for (ice::devui::DevUIWidget* widget : _inactive_widgets)
            {
                ice::array::push_back(_widgets, { .widget = widget, .state = _allocator.create<WidgetState>() });
            }
            ice::array::clear(_inactive_widgets);

            for (WidgetRuntimeInfo& info : _widgets)
            {
                info.widget->on_prepare(_render_trait->imgui_context(), *info.state);
            }
        }
    }

    void ImGuiSystem::register_trait(ice::TraitArchive& archive) noexcept
    {
        archive.register_trait(
            ice::TraitDescriptor
            {
                .name = ice::Constant_TraitName_DevUI,
                .fn_factory = ice::devui::detail::create_imgui_trait,
                .fn_register = nullptr,
                .fn_unregister = nullptr,
                .required_dependencies = { },
                .optional_dependencies = { },
                .fn_factory_userdata = this,
            }
        );
    }

    void ImGuiSystem::register_widget(ice::devui::DevUIWidget* widget) noexcept
    {
        if (_render_trait != nullptr)
        {
            WidgetState* state = _allocator.create<WidgetState>();
            widget->on_prepare(_render_trait->imgui_context(), *state);
            ice::array::push_back(_widgets, { .widget = widget, .state = state });
        }
        else
        {
            ice::array::push_back(_inactive_widgets, widget);
        }
    }

    void ImGuiSystem::unregister_widget(ice::devui::DevUIWidget* widget) noexcept
    {
        if (_render_trait == nullptr)
        {
            return;
        }

        ice::u32 const count = ice::array::count(_widgets);
        if (count == 0)
        {
            return;
        }

        ice::u32 idx = 0;
        for (; idx < count; ++idx)
        {
            if (_widgets[idx].widget == widget)
            {
                break;
            }
        }

        WidgetState* state = _widgets[idx].state;
        if (idx < (count - 1))
        {
            _widgets[idx] = _widgets[count - 1];
        }
        _allocator.destroy(state);

        ice::array::pop_back(_widgets);
    }

    void ImGuiSystem::set_alloc_tree_widget(ice::devui::ImGui_AllocatorTreeWidget* widget) noexcept
    {
        _widget_alloc_tree = widget;
        register_widget(_widget_alloc_tree);
    }

    void ImGuiSystem::render_builtin_widgets(ice::EngineFrame& frame) noexcept
    {
        if (_render_trait == nullptr)
        {
            return;
        }

        ice::String categories[]{
            "File",
            "Settings",
            "Tools",
            "Test",
            "Uncategorized"
        };

        static bool show_demo = false;

        if (_render_trait->start_frame())
        {
            if (ImGui::BeginMainMenuBar())
            {
                for (ice::String category : categories)
                {
                    if (ImGui::BeginMenu(ice::string::begin(category)))
                    {
                        for (WidgetRuntimeInfo& info : _widgets)
                        {
                            WidgetState& state = *info.state;
                            WidgetSettings const& settings = info.widget->settings();

                            if (settings.menu_category == category && ice::string::any(settings.menu_text))
                            {
                                ImGui::MenuItem(ice::string::begin(settings.menu_text), nullptr, &state.is_visible);
                            }
                        }

                        // Special case for ImGui Demo
                        if (category == "Uncategorized")
                        {
                            ImGui::MenuItem("ImGui Demo Window", nullptr, &show_demo);
                        }
                        ImGui::EndMenu();
                    }
                }
            }
            ImGui::EndMainMenuBar();

            if (show_demo)
            {
                ImGui::ShowDemoWindow(&show_demo);
            }

            for (WidgetRuntimeInfo& info : _widgets)
            {
                if (info.state->is_visible)
                {
                    info.widget->on_draw();
                }
            }
        }
        _render_trait->end_frame(frame);
    }

} // namespace ice::devui
