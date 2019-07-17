#pragma once
#include <core/message/buffer.hxx>
#include <core/pod/collections.hxx>
#include <SDL.h>

namespace driver::sdl2
{


    //! \brief Represents a transform object which is able to create a message object.
    class EventTransform
    {
    public:
        virtual ~EventTransform() noexcept = default;

        //! \brief Tries to transform the given event and returns true if successful.
        virtual bool transform_event(const SDL_Event&, core::MessageBuffer& message_buffer) noexcept = 0;
    };

    //! \brief Processes a single SDL event and pushes the result on the given message buffer.
    void process_event(
        const core::pod::Array<EventTransform*>& transforms
        , const SDL_Event& sdl_event
        , core::MessageBuffer& message_buffer
    ) noexcept;


} // namespace driver::sdl2
