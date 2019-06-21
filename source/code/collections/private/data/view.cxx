#include <core/data/view.hxx>

namespace core
{

data_view::data_view(const void* data, uint32_t size) noexcept
    : _data{ data }
    , _size{ size }
{
}

} // namespace core
