#include <input/utils/message_queue.hxx>
#include <core/memory.hxx>

#include <cassert>

namespace input::message
{
    namespace detail
    {
        //! \brief An entry in the message queue.
        struct MessageHeader
        {
            //! \brief Message metadata
            Metadata metadata;

            //! \brief Message data size.
            uint32_t data_size;
        };

        //! \brief Gets the header entry from the given pointer value.
        auto get_header(void* buffer) noexcept -> MessageHeader*
        {
            return reinterpret_cast<MessageHeader*>(core::memory::utils::align_forward(buffer, alignof(MessageHeader)));
        }

        //! \brief Gets the header entry from the given pointer value.
        auto get_header(const void* buffer) noexcept -> const MessageHeader*
        {
            return reinterpret_cast<const MessageHeader*>(core::memory::utils::align_forward(buffer, alignof(MessageHeader)));
        }

        //! \brief Searches for the next header using the previous one.
        auto next_header(const MessageHeader* previous) noexcept -> const MessageHeader*
        {
            return reinterpret_cast<const MessageHeader*>(core::memory::utils::align_forward(
                core::memory::utils::pointer_add(previous, sizeof(MessageHeader) + previous->data_size)
                , alignof(MessageHeader)
            ));
        }

        //! \brief Sets the data for the given message.
        auto set_data(MessageHeader* header, core::data_view data) noexcept
        {
            void* data_location = core::memory::utils::pointer_add(header, sizeof(MessageHeader));
            std::memcpy(data_location, data._data, data._size);

            // Save the size of the data buffer in the header so we can later traverse it.
            header->data_size = data._size;
        }

        //! \brief Returns a view into the data of a given message.
        auto get_data(const MessageHeader* header) noexcept -> core::data_view
        {
            auto* data_location = core::memory::utils::pointer_add(header, sizeof(MessageHeader));
            return { data_location, header->data_size };
        }
    }


    Queue::Queue(core::allocator& alloc) noexcept
        : _allocator{ alloc }
        , _data{ _allocator }
    { }

    Queue::~Queue() noexcept
    {
        core::buffer::set_capacity(_data, 0);
    }

    void Queue::clear() noexcept
    {
        core::buffer::clear(_data);
        _count = 0;
    }

    void Queue::push(Metadata metadata, core::data_view data) noexcept
    {
        const auto required_size = static_cast<uint32_t>(core::buffer::size(_data) + sizeof(detail::MessageHeader) + data._size);

        // Reserve enough memory
        core::buffer::reserve(_data, required_size);

        auto* header = detail::get_header(core::buffer::end(_data));
        header->metadata = metadata;

        // Set the data buffer
        detail::set_data(header, data);

        // Resize the buffer
        core::buffer::resize(_data, required_size);

        // Increment the message counter
        _count += 1;
    }

    void Queue::visit(std::function<void(const Metadata&, core::data_view data)> callback) const noexcept
    {
        auto* current_header = detail::get_header(core::buffer::begin(_data));

        uint32_t visited_messages = 0;
        while (visited_messages < _count)
        {
            IS_ASSERT(
                core::memory::utils::pointer_distance(current_header, core::buffer::end(_data)) >= sizeof(detail::MessageHeader)
                , "Invalid message header location!"
            );

            callback(current_header->metadata, detail::get_data(current_header));

            // Get the next header
            current_header = detail::next_header(current_header);
            visited_messages += 1;
        }
    }


} // namespace input::message
