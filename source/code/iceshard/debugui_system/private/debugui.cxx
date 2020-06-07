#include <debugui/debugui.hxx>
#include <core/debug/assert.hxx>
#include <imgui/imgui.h>

#include <core/platform/windows.hxx>

namespace iceshard::debug
{

    namespace detail
    {

        using CreateDebugUIFunc = auto(core::allocator&, iceshard::debug::debugui_context_handle) -> DebugUI*;
        using ReleaseDebugUIFunc = void(core::allocator&, DebugUI*);

        auto get_imgui_context(debugui_context_handle handle) noexcept -> ImGuiContext*
        {
            return reinterpret_cast<ImGuiContext*>(handle);
        }

        class ModuleDebugUI : public DebugUI
        {
        public:
            ModuleDebugUI(
                core::allocator& alloc,
                debugui_context_handle context,
                DebugUI* object,
                ReleaseDebugUIFunc* release_func
            )
                : DebugUI{ context }
                , _allocator{ alloc }
                , _wrapped_object{ object }
                , _release_func{ release_func }
            {

            }

            ~ModuleDebugUI() noexcept
            {
                _release_func(_allocator, _wrapped_object);
            }

            void update(core::MessageBuffer const& messages) noexcept override
            {
                _wrapped_object->update(messages);
            }

            void begin_frame() noexcept override
            {
                _wrapped_object->begin_frame();
            }

            void end_frame() noexcept override
            {
                _wrapped_object->end_frame();
            }

        private:
            core::allocator& _allocator;
            DebugUI* _wrapped_object;
            ReleaseDebugUIFunc* _release_func;
        };

    } // namespace detail

    DebugUI::DebugUI(debugui_context_handle context_handle) noexcept
    {
        ImGuiContext* const context = detail::get_imgui_context(context_handle);
        bool const is_same_context = ImGui::GetCurrentContext() == context;
        bool const is_null_context = ImGui::GetCurrentContext() == nullptr;

        IS_ASSERT(is_same_context || is_null_context, "Unexpected ImGui context value!");
        ImGui::SetCurrentContext(context);
    }

    auto load_debugui_from_module(
        core::allocator& alloc,
        core::ModuleHandle module,
        debugui_context_handle context
    ) noexcept -> core::memory::unique_pointer<DebugUI>
    {
        core::memory::unique_pointer<DebugUI> result{ nullptr, { alloc } };

        HMODULE module_handle = reinterpret_cast<HMODULE>(module);

        void* create_func_ptr = GetProcAddress(module_handle, "create_debugui");
        void* release_func_ptr = GetProcAddress(module_handle, "release_debugui");

        if (create_func_ptr != nullptr && release_func_ptr != nullptr)
        {
            auto* create_func = reinterpret_cast<detail::CreateDebugUIFunc*>(create_func_ptr);
            auto* release_func = reinterpret_cast<detail::ReleaseDebugUIFunc*>(release_func_ptr);

            if (auto* obj = create_func(alloc, context); obj != nullptr)
            {
                result = core::memory::make_unique<DebugUI, detail::ModuleDebugUI>(alloc, alloc, context, obj, release_func);
            }
        }
        return result;
    }

} // namespace debugui
