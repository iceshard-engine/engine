/// Copyright 2022 - 2025, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#include "imgui_system.hxx"
#include "imgui_trait.hxx"

#include <ice/mem_allocator.hxx>
#include <ice/log_module.hxx>
#include <ice/log_sink.hxx>
#include <ice/devui_module.hxx>
#include <ice/world/world_trait_module.hxx>

namespace ice::devui
{

    static ImGuiSystem* global_ImGuiContext = nullptr;
    static ProxyAllocator* global_ImGuiAllocator = nullptr;

    static auto imgui_memalloc(size_t size, void* userdata) noexcept -> void*
    {
        return reinterpret_cast<ice::ProxyAllocator*>(userdata)->allocate(ice::usize{size}).memory;
    }

    static auto imgui_memfree(void* ptr, void* userdata) noexcept -> void
    {
        return reinterpret_cast<ice::ProxyAllocator*>(userdata)->deallocate(ptr);
    }

    void imgui_logger_sink(void* userdata, ice::LogSinkMessage const& message) noexcept
    {
        reinterpret_cast<ImGuiSystem*>(userdata)->logger().add_entry(message);
    }

    auto imgui_create_context(ice::Allocator& alloc) noexcept -> ice::DevUIContext*
    {
        ICE_ASSERT_CORE(global_ImGuiContext == nullptr);

        global_ImGuiContext = alloc.create<ImGuiSystem>(alloc);

        ice::log_module_register_sink(imgui_logger_sink, global_ImGuiContext);
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
        DevUIContextSetupParams const params{
            .native_context = ImGui::GetCurrentContext(),
            .fn_alloc = imgui_memalloc,
            .fn_dealloc = imgui_memfree,
            .alloc_userdata = global_ImGuiAllocator
        };
        if (cb("devui-context/imgui"_sid, params, userdata) == false)
        {
            ICE_LOG(LogSeverity::Warning, LogTag::System, "Failed to initialize 'ImGui' context on module!");
        }
    }

    constexpr auto imgui_trait_name() noexcept -> ice::StringID
    {
        return "devui.world-trait.imgui"_sid;
    }

    void imgui_setup_mainmenu(ice::Span<ice::String> categories) noexcept
    {
        if (global_ImGuiContext != nullptr)
        {
            global_ImGuiContext->setup_mainmenu(categories);
        }
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

    void imgui_remove_widget(ice::DevUIWidget* widget) noexcept
    {
        ICE_LOG_IF(
            global_ImGuiContext != nullptr,
            LogSeverity::Error, LogTag::System,
            "Trying to remove DevUI widget without a valid context!"
        );
        if (global_ImGuiContext != nullptr)
        {
            global_ImGuiContext->unregister_widget(widget);
        }
    }

    struct ImGuiDevUIModule : ice::Module<ImGuiDevUIModule>
    {
        static void v1_devui_system(ice::api::DevUI_API& api) noexcept
        {
            api.fn_create_context = imgui_create_context;
            api.fn_destry_context = imgui_destroy_context;
            api.fn_context_setup = imgui_context_setup;
            api.fn_context_setup_menu = imgui_setup_mainmenu;
            api.fn_context_register_widget = imgui_register_widget;
            api.fn_context_remove_widget = imgui_remove_widget;
            api.fn_context_trait_name = imgui_trait_name;
        }

        static bool on_load(ice::Allocator& alloc, ice::ModuleNegotiator auto const& negotiator) noexcept
        {
            ICE_ASSERT_CORE(global_ImGuiAllocator == nullptr);
            global_ImGuiAllocator = alloc.create<ice::ProxyAllocator>(alloc, "ImGUI");

            ImGui::SetAllocatorFunctions(imgui_memalloc, imgui_memfree, global_ImGuiAllocator);
            ImGui::CreateContext();

            ice::LogModule::init(alloc, negotiator);
            return negotiator.register_api(v1_devui_system);
        }

        static void on_unload(ice::Allocator& alloc) noexcept
        {
            ImGui::DestroyContext();

            alloc.destroy(global_ImGuiAllocator);
            global_ImGuiAllocator = nullptr;
        }

        IS_WORKAROUND_MODULE_INITIALIZATION(ImGuiDevUIModule);
    };

    struct ImGuiTraitModule : ice::Module<ImGuiTraitModule>
    {
        static auto imgui_trait_factory(ice::Allocator& alloc, ice::TraitContext& ctx, void* userdata) noexcept -> UniquePtr<ice::Trait>
        {
            ICE_ASSERT_CORE(userdata == global_ImGuiContext);
            return ctx.make_unique<ImGuiTrait>(alloc, alloc, *global_ImGuiContext);
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

        static bool on_load(ice::Allocator& alloc, ice::ModuleNegotiator auto const& negotiator) noexcept
        {
            return negotiator.register_api(v1_traits_api);
        }

        IS_WORKAROUND_MODULE_INITIALIZATION(ImGuiTraitModule);
    };

} // namespace ice
