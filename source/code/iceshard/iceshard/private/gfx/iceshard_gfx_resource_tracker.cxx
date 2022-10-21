#include "iceshard_gfx_resource_tracker.hxx"
#include <ice/assert.hxx>

namespace ice::gfx
{

    IceGfxResourceTracker::IceGfxResourceTracker(ice::Allocator& alloc) noexcept
        : _resources{ alloc }
    {
    }

    void IceGfxResourceTracker::track_resource(
        ice::StringID_Arg name,
        ice::gfx::GfxResource resource
    ) noexcept
    {
        //ICE_ASSERT(
        //    ice::pod::hash::has(_resources, ice::hash(name)) == false,
        //    "A resource with this name {} is already tracked!",
        //    ice::stringid_hint(name)
        //);

        ice::hashmap::set(
            _resources,
            ice::hash(name),
            resource
        );
    }

    auto IceGfxResourceTracker::find_resource(
        ice::StringID_Arg name,
        ice::gfx::GfxResource::Type type
    ) noexcept -> ice::gfx::GfxResource
    {
        GfxResource const result = ice::hashmap::get(
            _resources,
            ice::hash(name),
            GfxResource{ .type = GfxResource::Type::Invalid }
        );

        ICE_ASSERT(
            result.type == type,
            "Resource with name {} is not valid, result resource type does not match required type!",
            ice::stringid_hint(name)
        );
        return result;
    }

} // namespace ice::gfx

