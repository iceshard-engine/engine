#include <core/data/view.hxx>
#include <core/debug/assert.hxx>

namespace core
{

    data_view_aligned::data_view_aligned(void const* data, uint32_t size, uint32_t alignment) noexcept
        : _data{ data }
        , _size{ size }
        , _align{ alignment }
    {
        IS_ASSERT((reinterpret_cast<std::uintptr_t>(_data) % _align) == 0, "The 'data' pointer is not properly aligned! [ expected alignment: {} ]", _align);
    }

} // namespace core
