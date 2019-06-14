#pragma once
#include <memsys/allocator.hxx>

#include <utility>
#include <memory>

namespace memsys
{


namespace detail
{

//! \brief A unqiue_pointer deleter for objects allocated with memsys::allocator.
template<class T>
class memsys_deleter
{
public:
    //! \brief Empty deleter objects are not allowed.
    memsys_deleter() noexcept = delete;

    //! \brief Creating a deleter from an allocator.
    memsys_deleter(memsys::allocator& alloc) noexcept;

    //! \brief Creating a deleter from another deleter.
    memsys_deleter(memsys_deleter&& other) noexcept;

    //! \brief Updating this deleter from another deleter. //#todo is this required?
    auto operator=(memsys_deleter&& other) noexcept -> memsys_deleter&;

    //! \brief Method required by the unique_ptr deleter concept.
    void operator()(T* object) noexcept;

private:
    // A nullptr shouldn't be possible, so it will be seen as invalid.
    mem::allocator* _allocator{ nullptr };
};

} // namespace detail


//! \brief The memsys aware uniue_pointer type.
template<class T>
using unique_pointer = std::unique_ptr<T, detail::memsys_deleter<T>>;

//! \brief The make_unique function with an mandatory allocator object.
template<class T, class... Args>
auto make_unique(memsys::allocator& alloc, Args&&... args) noexcept -> unique_pointer<T>;


#include "pointer.inl"


} // namespace mem
