#include "event.hxx"

#include <core/pod/array.hxx>
#include <algorithm>

namespace driver::sdl2
{

    void process_event(const core::pod::Array<EventTransform*>& transforms, const SDL_Event& sdl_event, core::MessageBuffer& message_buffer) noexcept
    {
        auto it = core::pod::array::begin(transforms);
        const auto end = core::pod::array::end(transforms);

        while (it != end)
        {
            (*it)->transform_event(sdl_event, message_buffer);
            ++it;
        }
    }

} // namespace driver::sdl2
