#pragma once
#include <core/allocator.hxx>
#include <core/string_view.hxx>
#include <core/pointer.hxx>

namespace render
{

    class RenderSystem;

    //! \brief Describes the engine module.
    class RenderSystemModule
    {
    public:
        virtual ~RenderSystemModule() noexcept = default;

        //! \brief Returns the engine object from the loaded module.
        [[nodiscard]] virtual auto render_system() noexcept -> RenderSystem* = 0;

        //! \brief Returns the engine object from the loaded module.
        [[nodiscard]] virtual auto render_system() const noexcept -> const RenderSystem* = 0;
    };

    //! \brief Tries to load the engine module from the given library path.
    [[nodiscard]] auto load_render_system_module(
        core::allocator& alloc,
        core::StringView path) noexcept -> core::memory::unique_pointer<RenderSystemModule>;

} // namespace render
