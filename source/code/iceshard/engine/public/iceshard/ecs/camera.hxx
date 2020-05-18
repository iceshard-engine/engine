#pragma once
#include <iceshard/math.hxx>
#include <iceshard/component/component_system.hxx>

namespace iceshard::component
{

    struct Camera
    {
        static constexpr auto identifier = "isc.camera"_sid;

        //! \brief World position of the camera.
        ism::vec3f position;

        //! \brief Where the camera is looking.
        ism::vec3f front;

        //! \brief Field of View (in degrees)
        ism::f32 fovy;

        //! \brief Yaw and Pitch
        ism::f32 yaw;
        ism::f32 pitch;
    };

} // namespace iceshard::ecs
