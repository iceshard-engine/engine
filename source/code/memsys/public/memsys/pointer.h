#pragma once
#include "memsys/allocator.h"

#include <utility>
#include <memory>

namespace memsys
{


/// Custom unique_pointer deleter ///


namespace detail
{

//! \brief A unqiue_pointer deleter for objects allocated with memsys::allocator.
template<class T>
class mooned_deleter
{
public:
    //! \brief Empty deleter objects are not allowed.
    mooned_deleter() noexcept = delete;

    //! \brief Creating a deleter from an allocator.
    mooned_deleter(memsys::allocator& alloc) noexcept;

    //! \brief Creating a deleter from another deleter.
    mooned_deleter(mooned_deleter&& other) noexcept;

    //! \brief Updating this deleter from another deleter. //#todo is this required?
    auto operator=(mooned_deleter&& other) noexcept -> mooned_deleter&;

    //! \brief Method required by the unique_ptr deleter concept.
    void operator()(T* object) noexcept;

private:
    // A nullptr shouldn't be possible, so it will be seen as invalid.
    mem::allocator* _allocator{ nullptr };
};

} // namespace detail


/// Custom unique_pointer types ///


//! \brief The memsys aware uniue_pointer type.
template<class T>
using unique_pointer = std::unique_ptr<T, detail::mooned_deleter<T>>;

//! \brief The make_unique function with an mandatory allocator object.
template<class T, class... Args>
auto make_unique(memsys::allocator& alloc, Args&&... args) noexcept -> unique_pointer<T>;


/// Inline implementation ///


#include "pointer.inl"


} // namespace mem
