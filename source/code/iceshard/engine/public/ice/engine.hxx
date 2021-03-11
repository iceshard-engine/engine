#pragma once
#include <ice/unique_ptr.hxx>
#include <ice/gfx/gfx_types.hxx>

namespace ice
{

    class EngineRunner;

    class Engine
    {
    public:
        virtual ~Engine() noexcept = default;

        virtual auto create_runner(
            ice::gfx::GfxDeviceCreateInfo const& gfx_create_info
        ) noexcept -> ice::UniquePtr<EngineRunner> = 0;
    };

} // namespace ice
