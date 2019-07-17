#pragma once
#include <core/message/buffer.hxx>
#include <core/string_types.hxx>
#include <core/pointer.hxx>

namespace media
{


    //! \brief Describes a media driver, which is responsible to handle low-level media events and return proper messages.
    class MediaDriver
    {
    public:
        virtual ~MediaDriver() noexcept = default;

        //! \brief Updates the internal media driver state.
        virtual void update() noexcept = 0;

        //! \brief Queries the media driver for messages.
        virtual void query_messages(core::MessageBuffer& message_buffer) const noexcept = 0;
    };


    //! \brief Describes a driver module object.
    class DriverModule
    {
    public:
        virtual ~DriverModule() noexcept = default;

        //! \brief Returns the associated media driver.
        [[nodiscard]]
        virtual auto media_driver() noexcept -> MediaDriver* = 0;

        //! \brief Returns the associated media driver.
        [[nodiscard]]
        virtual auto media_driver() const noexcept -> const MediaDriver* = 0;
    };


    //! \brief Tries to load a device deiver from the given module path.
    [[nodiscard]]
    auto load_driver_module(core::allocator& alloc, core::StringView<> driver_module_path) noexcept -> core::memory::unique_pointer<DriverModule>;


} // namespace driver