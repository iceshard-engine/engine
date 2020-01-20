#pragma once
#include <core/allocator.hxx>
#include <core/pointer.hxx>
#include <core/string_types.hxx>
#include <core/message/operations.hxx>

namespace input
{
    class InputSystem;
} // namespace input

namespace asset
{
    class AssetSystem;
} // namespace asset

namespace render
{
    class RenderSystem;
} // namespace render

namespace debugui
{

    enum class debugui_context_handle : uintptr_t;

    class DebugUI;

    class DebugUIContext
    {
    public:
        virtual ~DebugUIContext() noexcept = default;

        virtual void register_ui(DebugUI* ui_object) noexcept = 0;

        virtual void update(core::MessageBuffer const& messages) noexcept = 0;

        virtual void begin_frame() noexcept = 0;

        virtual void end_frame() noexcept = 0;

        virtual auto context_handle() const noexcept -> debugui_context_handle = 0;
    };

    class DebugUIModule
    {
    public:
        virtual ~DebugUIModule() noexcept = default;

        virtual auto context() noexcept -> DebugUIContext& = 0;

        virtual auto context_handle() noexcept -> debugui_context_handle = 0;
    };

    auto load_module(
        core::allocator& alloc,
        core::StringView path,
        input::InputSystem& input_system,
        asset::AssetSystem& asset_system,
        render::RenderSystem& render_system
    ) noexcept -> core::memory::unique_pointer<DebugUIModule>;

} // namespace debugui
