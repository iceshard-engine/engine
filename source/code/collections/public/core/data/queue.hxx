#pragma once
#include <core/allocator.hxx>
#include <core/data/buffer.hxx>

#include <functional>

namespace core
{


    //! \brief A queue object for messages.
    class data_queue final
    {
    public:
        data_queue(core::allocator& alloc) noexcept;
        ~data_queue() noexcept;

        //! \brief Clears the queue.
        void clear() noexcept;

        //! \brief Pushes a new message with the given metadata and data.
        void push(core::data_view data) noexcept;

        //! \brief The current message count.
        auto count() const noexcept -> uint32_t { return _count; }


    public:
        //! \brief A iterator for the raw data queue.
        class Iterator final
        {
        public:
            Iterator(const data_queue& queue) noexcept;
            Iterator(const data_queue& queue, bool is_end) noexcept;
            ~Iterator() noexcept;

            //! \brief Defines equality operators.
            bool operator==(const Iterator& other) noexcept;
            bool operator!=(const Iterator& other) noexcept { return !(*this == other); }

            //! \brief Moves the iterator forward.
            void operator++() noexcept;

            //! \brief Allows to dereference the value.
            auto operator*() const noexcept -> core::data_view;

        private:
            //! \brief Additional data.
            const void* _data;

            //! \brief The current element.
            uint32_t _element;
        };

        //! \brief Begin iterator for this data queue.
        auto begin() const noexcept->Iterator;

        //! \brief End iterator for this data queue.
        auto end() const noexcept -> Iterator;


    private:
        core::allocator& _allocator;

        //! \brief Data buffer.
        core::Buffer _data;

        //! \brief Message count.
        uint32_t _count{ 0 };
    };


} // namespace core::data
