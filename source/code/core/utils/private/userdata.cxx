#include <ice/userdata.hxx>
#include <ice/assert.hxx>

namespace ice
{

    void UserdataBase<true>::validate(std::type_info const& info) const noexcept
    {
        ICE_ASSERT(
            _internal == info.hash_code(),
            "Trying to cast the userdata pointer to a unrelated type!"
        );
    }

} // namespace ice
