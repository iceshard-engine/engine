#pragma once
#include <core/pointer.hxx>
#include <iceshard/renderer/render_module.hxx>
#include <iceshard/renderer/render_system.hxx>

namespace iceshard::renderer
{

    class RenderModule
    {
    public:
        virtual ~RenderModule() noexcept = default;

        auto render_system() -> RenderSystem*;

        auto render_system() const -> RenderSystem const*;

    protected:
        virtual auto render_module_interface() const noexcept -> api::RenderModuleInterface* = 0;
    };

    //! \brief Describes the engine module.
    class RenderModuleInstance
    {
    public:
        virtual ~RenderModuleInstance() noexcept = default;

        //! \brief Returns the render system object from the loaded module.
        [[nodiscard]]
        virtual auto render_module() noexcept -> iceshard::renderer::RenderModule* = 0;

        //! \brief Returns the render system object from the loaded module.
        [[nodiscard]]
        virtual auto render_module() const noexcept -> iceshard::renderer::RenderModule const* = 0;
    };

    //! \brief Tries to load the engine module from the given library path.
    [[nodiscard]]
    auto load_render_system_module(
        core::allocator& alloc,
        core::StringView path
    ) noexcept -> core::memory::unique_pointer<RenderModuleInstance>;

} // namespace iceshard::renderer

