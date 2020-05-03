#pragma once
#include <core/math/vector.hxx>
#include <iceshard/component/component_system.hxx>

namespace iceshard::component
{

    struct Camera
    {
        static constexpr auto identifier = "isc.camera"_sid;

        //! \brief World position of the camera.
        core::math::vec3 position;

        //! \brief Where the camera is looking.
        core::math::vec3 front;

        //! \brief Field of View (in degrees)
        core::math::f32 fovy;

        //! \brief Yaw and Pitch
        core::math::f32 yaw;
        core::math::f32 pitch;
    };

} // namespace iceshard::ecs
