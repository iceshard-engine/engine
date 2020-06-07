#pragma once
#include <core/allocator.hxx>

#include <utility>
#include <memory>

namespace core::memory
{

    namespace detail
    {

        //! \brief A unqiue_pointer deleter for objects allocated with memsys::allocator.
        template<class T>
        class memsys_default_deleter
        {
        public:
            //! \brief Empty deleter objects are not allowed.
            memsys_default_deleter() noexcept = delete;

            //! \brief Creating a deleter from an allocator.
            memsys_default_deleter(core::allocator& alloc) noexcept;

            //! \brief Creating a deleter from another deleter.
            memsys_default_deleter(memsys_default_deleter&& other) noexcept;

            //! \brief Updating this deleter from another deleter. //#todo is this required?
            auto operator=(memsys_default_deleter&& other) noexcept -> memsys_default_deleter&;

            //! \brief Method required by the unique_ptr deleter concept.
            void operator()(T* object) noexcept;

        private:
            // A nullptr shouldn't be possible, so it will be seen as invalid.
            core::allocator* _allocator{ nullptr };
        };

        //! \brief A unqiue_pointer deleter for objects allocated with memsys::allocator.
        template<typename T>
        class memsys_custom_deleter
        {
            using CustomDeleterFunction = void(core::allocator&, T*);
        public:
            //! \brief Empty deleter objects are not allowed.
            memsys_custom_deleter() noexcept = delete;

            //! \brief Creating a deleter from an allocator.
            memsys_custom_deleter(core::allocator& alloc, CustomDeleterFunction* func) noexcept;

            //! \brief Creating a deleter from another deleter.
            memsys_custom_deleter(memsys_custom_deleter&& other) noexcept;

            //! \brief Updating this deleter from another deleter. //#todo is this required?
            auto operator=(memsys_custom_deleter&& other) noexcept -> memsys_custom_deleter&;

            //! \brief Method required by the unique_ptr deleter concept.
            void operator()(T* object) noexcept;

        private:
            core::allocator* _allocator{ nullptr };
            CustomDeleterFunction* _deleter{ nullptr };
        };

    } // namespace detail

    //! \brief An allocator aware uniue_pointer type.
    template<typename T, typename D = detail::memsys_default_deleter<T>>
    using unique_pointer = std::unique_ptr<T, D>;

    //! \brief The make_unique function with an mandatory allocator object.
    template<typename Result, typename Type = Result, typename... Args>
    auto make_unique(core::allocator& alloc, Args&&... args) noexcept -> unique_pointer<Result>;

} // namespace core::memory

#include "pointer.inl"
