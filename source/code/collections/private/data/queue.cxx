#include <core/data/queue.hxx>
#include <core/memory.hxx>

#include <cassert>

namespace core::data
{
    namespace detail
    {
        //! \brief An entry in the message queue.
        struct MessageHeader
        {
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

    void Queue::push(core::data_view data) noexcept
    {
        // We need to know the last header to properly calculate the required size.
        const auto* end_header = detail::get_header(
            core::memory::utils::pointer_add(
                core::buffer::begin(_data)
                , core::buffer::size(_data) + sizeof(detail::MessageHeader) + data._size
            )
        );

        const auto required_size = core::memory::utils::pointer_distance(core::buffer::begin(_data), end_header);

        // Reserve enough memory
        core::buffer::reserve(_data, required_size);

        auto* header = detail::get_header(core::buffer::end(_data));

        // Set the data buffer
        detail::set_data(header, data);

        // Resize the buffer
        core::buffer::resize(_data, required_size);

        // Increment the message counter
        _count += 1;
    }

    auto Queue::begin() const noexcept -> Iterator
    {
        return Iterator{ *this };
    }

    auto Queue::end() const noexcept -> Iterator
    {
        return Iterator{ *this, true };
    }


    Queue::Iterator::Iterator(const Queue& queue) noexcept
        : _data{ core::buffer::begin(queue._data) }
        , _element{ 0 }
    { }

    Queue::Iterator::Iterator(const Queue& queue, bool) noexcept
        : _data{ nullptr }
        , _element{ queue.count() }
    { }

    Queue::Iterator::~Iterator() noexcept
    { }

    bool Queue::Iterator::operator==(const Iterator& other) noexcept
    {
        return _element == other._element;
    }

    void Queue::Iterator::operator++() noexcept
    {
        _data = detail::next_header(detail::get_header(_data));
        _element += 1;
    }

    auto Queue::Iterator::operator*() const noexcept -> core::data_view
    {
        auto* header = detail::get_header(_data);
        return { reinterpret_cast<const void*>(header + 1), header->data_size };
    }

} // namespace core::data
