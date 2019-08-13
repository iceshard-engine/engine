#pragma once
#include <render_system/command_buffer.hxx>

namespace render
{


    //! \brief Clear command.
    void clear(RenderCommandBuffer& buffer, float r, float g, float b) noexcept;


    //! \brief Render command data structures.
    namespace command_data
    {

        struct Clear
        {
            static CommandName command_name;

            float r, g, b;
        };

    } //! \brief Command data.


} // namespace render
