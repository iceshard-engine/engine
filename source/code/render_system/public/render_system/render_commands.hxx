#pragma once
#include <render_system/render_command_buffer.hxx>

namespace render
{


    //! \brief Data structures for render commands.
    namespace data
    {


        struct Clear
        {
            static CommandName command_name;

            float r, g, b;
        };


    } // namespace data


    //! \brief Commands which can be added to the buffer.
    namespace command
    {


        //! \brief Clear command.
        void clear(RenderCommandBuffer& buffer, const data::Clear& data) noexcept;


    } // namespace command


} // namespace render
