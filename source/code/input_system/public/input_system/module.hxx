#pragma once
#include <core/message/buffer.hxx>
#include <core/string_types.hxx>
#include <core/pointer.hxx>

namespace input
{


    //! \brief Describes a media driver, which is responsible to handle low-level media events and return proper messages.
    class InputQuery
    {
    public:
        virtual ~InputQuery() noexcept = default;

        //! \brief Updates the internal media driver state.
        virtual void update() noexcept = 0;

        //! \brief Queries the media driver for messages.
        virtual void query_messages(core::MessageBuffer& message_buffer) const noexcept = 0;
    };


    //! \brief Describes a driver module object.
    class InputModule
    {
    public:
        virtual ~InputModule() noexcept = default;

        //! \brief Returns the associated media driver.
        [[nodiscard]]
        virtual auto media_driver() noexcept -> InputQuery* = 0;

        //! \brief Returns the associated media driver.
        [[nodiscard]]
        virtual auto media_driver() const noexcept -> const InputQuery* = 0;
    };


    //! \brief Tries to load a device deiver from the given module path.
    [[nodiscard]]
    auto load_driver_module(core::allocator& alloc, core::StringView<> driver_module_path) noexcept -> core::memory::unique_pointer<InputModule>;


} // namespace input