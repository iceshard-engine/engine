#pragma once
#include <core/allocator.hxx>
#include <core/data/queue.hxx>
#include <core/pod/collections.hxx>
#include <core/cexpr/stringid.hxx>

namespace render
{

    //! \brief A command identifier.
    struct CommandName
    {
        core::cexpr::stringid_type identifier;
    };

    static_assert(std::is_trivially_copyable_v<CommandName>, "The 'CommandId' type needs to be trivially copyable to work properly.");

    //! \brief A buffer with render commands tightly stored.
    class RenderCommandBuffer final
    {
    public:
        RenderCommandBuffer(core::allocator& alloc) noexcept;
        ~RenderCommandBuffer() noexcept;

        //! \brief Clears the command buffer.
        void clear() noexcept;

        //! \brief Pushes a new render command onto the buffer with the given data.
        void push(CommandName command, core::data_view_aligned command_data) noexcept;

        //! \brief Runs the given callback on every render command in the buffer.
        template<typename Func>
        void visit(Func&& func) const noexcept;

    private:
        core::pod::Array<CommandName> _command_list;
        core::data_queue _command_data;
    };

    template<typename Func>
    inline void RenderCommandBuffer::visit(Func&& func) const noexcept
    {
        uint32_t command_index = 0;
        for (const auto& command_data_view : _command_data)
        {
            func(_command_list[command_index], command_data_view);
            command_index += 1;
        }
    }

    namespace detail
    {

        template<typename T, typename = int>
        struct has_name_member : std::false_type
        {
        };

        template<typename T>
        struct has_name_member<T, decltype((void)T::command_name, 0)> : std::true_type
        {
        };

    } // namespace detail

    //! \brief Additional RenderCommandBuffer operations.
    namespace buffer
    {

        //! \brief Pushes the whole command structure onto the command buffer.
        template<typename T>
        void push(RenderCommandBuffer& command_buffer, const T& command) noexcept
        {
            static_assert(
                std::is_trivially_copyable_v<T>, "Message object not trivially copyable!");
            static_assert(
                detail::has_name_member<T>::value, "Message missing static member 'command_name'!");
            static_assert(
                std::is_same_v<std::remove_cv_t<decltype(T::command_name)>, render::CommandName>, "Invalid type of message member 'command_name', expected 'core::cexpr::stringid_type'!");

            command_buffer.push(T::command_name, { &command, sizeof(T), alignof(T) });
        }

    } // namespace buffer

} // namespace render
