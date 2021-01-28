#pragma once
#include <ice/unique_ptr.hxx>
#include <ice/pod/array.hxx>
#include <ice/input/device_queue.hxx>

namespace ice::platform
{

    struct Event;

    enum class RenderDriver;

    class RenderSurface;

    class App
    {
    public:
        virtual ~App() noexcept = default;

        virtual void handle_inputs(
            ice::input::DeviceQueue const& device_events
        ) noexcept = 0;

        virtual void update(
            ice::pod::Array<ice::platform::Event> const& events
        ) noexcept = 0;
    };

    class Container
    {
    public:
        virtual ~Container() noexcept = default;

        virtual auto create_surface(RenderDriver driver) noexcept -> ice::UniquePtr<ice::platform::RenderSurface> = 0;

        virtual auto run() noexcept -> ice::i32 = 0;
    };

    auto create_app_container(
        ice::Allocator& alloc,
        ice::UniquePtr<ice::platform::App> app
    ) noexcept -> ice::UniquePtr<ice::platform::Container>;

} // namespace ice::platform
