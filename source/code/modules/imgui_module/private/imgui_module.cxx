/// Copyright 2022 - 2024, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#include "imgui_system.hxx"
#include "imgui_trait.hxx"

#include <ice/mem_allocator.hxx>
#include <ice/log_module.hxx>
#include <ice/devui_module.hxx>
#include <ice/world/world_trait_module.hxx>

namespace ice::devui
{

    static ImGuiSystem* global_ImGuiContext = nullptr;

    auto imgui_create_context(ice::Allocator& alloc) noexcept -> ice::DevUIContext*
    {
        ICE_ASSERT_CORE(global_ImGuiContext == nullptr);

        global_ImGuiContext = alloc.create<ImGuiSystem>(alloc);
        return global_ImGuiContext;
    }

    void imgui_destroy_context(ice::DevUIContext* context) noexcept
    {
        ICE_ASSERT_CORE(global_ImGuiContext == context);
        global_ImGuiContext = nullptr;

        ice::Allocator& alloc = static_cast<ImGuiSystem*>(context)->allocator();
        alloc.destroy(context);
    }

    void imgui_context_setup(ice::api::DevUI_API::FnContextSetupCallback cb, void* userdata) noexcept
    {
        if (cb("devui-context/imgui"_sid, ImGui::GetCurrentContext(), userdata) == false)
        {
            ICE_LOG(LogSeverity::Warning, LogTag::System, "Failed to initialize 'ImGui' context on module!");
        }
    }

    constexpr auto imgui_trait_name() noexcept -> ice::StringID
    {
        return "devui.world-trait.imgui"_sid;
    }

    void imgui_register_widget(ice::DevUIWidget* widget) noexcept
    {
        ICE_LOG_IF(
            global_ImGuiContext != nullptr,
            LogSeverity::Error, LogTag::System,
            "Trying to register DevUI widget without a valid context!"
        );
        if (global_ImGuiContext != nullptr)
        {
            global_ImGuiContext->register_widget(widget);
        }
    }

    [[maybe_unused]]
    static auto imgui_memalloc(size_t size, void* userdata) noexcept -> void*
    {
        return reinterpret_cast<ice::ProxyAllocator*>(userdata)->allocate(ice::usize{size}).memory;
    }

    [[maybe_unused]]
    static auto imgui_memfree(void* ptr, void* userdata) noexcept -> void
    {
        return reinterpret_cast<ice::ProxyAllocator*>(userdata)->deallocate(ptr);
    }

    struct ImGuiDevUIModule : ice::Module<ImGuiDevUIModule>
    {
        static inline ice::ProxyAllocator* imgui_alloc = nullptr;

        static void v1_devui_system(ice::api::DevUI_API& api) noexcept
        {
            api.fn_create_context = imgui_create_context;
            api.fn_destry_context = imgui_destroy_context;
            api.fn_context_setup = imgui_context_setup;
            api.fn_context_register_widget = imgui_register_widget;
            api.fn_context_trait_name = imgui_trait_name;
        }

        static bool on_load(ice::Allocator& alloc, ice::ModuleNegotiator const& negotiator) noexcept
        {
            ICE_ASSERT_CORE(imgui_alloc == nullptr);
            imgui_alloc = alloc.create<ice::ProxyAllocator>(alloc, "ImGUI");

            ImGui::SetAllocatorFunctions(imgui_memalloc, imgui_memfree, imgui_alloc);
            ImGui::CreateContext();

            ice::LogModule::init(alloc, negotiator);
            return negotiator.register_api(v1_devui_system);
        }

        static void on_unload(ice::Allocator& alloc) noexcept
        {
            ImGui::DestroyContext();

            alloc.destroy(imgui_alloc);
            imgui_alloc = nullptr;
        }

        IS_WORKAROUND_MODULE_INITIALIZATION(ImGuiDevUIModule);
    };

    struct ImGuiTraitModule : ice::Module<ImGuiTraitModule>
    {
        static auto imgui_trait_factory(ice::Allocator& alloc, void* userdata) noexcept -> UniquePtr<ice::Trait>
        {
            ICE_ASSERT_CORE(userdata == global_ImGuiContext);
            return ice::make_unique<ImGuiTrait>(alloc, alloc, *global_ImGuiContext);
        }

        static bool imgui_register_trait(ice::TraitArchive& arch) noexcept
        {
            if (global_ImGuiContext == nullptr)
            {
                return false;
            }

            arch.register_trait({ .name = imgui_trait_name(), .fn_factory = imgui_trait_factory, .fn_factory_userdata = global_ImGuiContext });
            return true;
        }

        static void v1_traits_api(ice::detail::world_traits::TraitsModuleAPI& api) noexcept
        {
            api.register_traits_fn = imgui_register_trait;
        }

        static bool on_load(ice::Allocator& alloc, ice::ModuleNegotiator const& negotiator) noexcept
        {
            return negotiator.register_api(v1_traits_api);
        }

        IS_WORKAROUND_MODULE_INITIALIZATION(TestModule);
    };

} // namespace ice
