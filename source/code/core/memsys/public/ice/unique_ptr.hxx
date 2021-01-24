#pragma once
#include <ice/allocator.hxx>

namespace ice
{

    namespace detail
    {
        template<class T>
        class IceCustomDeleter;
    } // namespace detail

    template<typename T, typename D = detail::IceCustomDeleter<T>>
    using UniquePtr = std::unique_ptr<T, D>;

    template<typename Result, typename Type = Result, typename... Args>
    auto make_unique(ice::Allocator& alloc, Args&&... args) noexcept -> ice::UniquePtr<Result>;

    template<typename Result>
    auto make_unique_null() noexcept -> ice::UniquePtr<Result>;

    namespace detail
    {

        //! \brief A unqiue_pointer deleter for objects allocated with memsys::allocator.
        template<class T>
        class IceDefaultDeleter
        {
        public:
            //! \brief Empty deleter objects are not allowed.
            IceDefaultDeleter() noexcept = delete;

            //! \brief Creating a deleter from an allocator.
            IceDefaultDeleter(ice::Allocator& alloc) noexcept;

            //! \brief Creating a deleter from another deleter.
            IceDefaultDeleter(IceDefaultDeleter&& other) noexcept;

            //! \brief Updating this deleter from another deleter. //#todo is this required?
            auto operator=(IceDefaultDeleter&& other) noexcept -> IceDefaultDeleter&;

            //! \brief Method required by the unique_ptr deleter concept.
            void operator()(T* object) noexcept;

        private:
            // A nullptr shouldn't be possible, so it will be seen as invalid.
            ice::Allocator* _allocator{ nullptr };
        };

        //! \brief A unqiue_pointer deleter for objects allocated with memsys::allocator.
        template<typename T>
        class IceCustomDeleter
        {
            using CustomDeleterFunction = void(ice::Allocator&, T*);
        public:
            //! \brief Empty deleter objects are not allowed.
            IceCustomDeleter() noexcept = delete;

            //! \brief Creating a deleter from an allocator.
            IceCustomDeleter(ice::Allocator& alloc) noexcept;

            //! \brief Creating a deleter from an allocator.
            IceCustomDeleter(ice::Allocator& alloc, CustomDeleterFunction* func) noexcept;

            //! \brief Creating a deleter from another deleter.
            IceCustomDeleter(IceCustomDeleter&& other) noexcept;

            //! \brief Updating this deleter from another deleter. //#todo is this required?
            auto operator=(IceCustomDeleter&& other) noexcept -> IceCustomDeleter&;

            //! \brief Method required by the unique_ptr deleter concept.
            void operator()(T* object) noexcept;

        protected:
            static void default_deleter_function(
                ice::Allocator& alloc,
                T* object
            ) noexcept;

        private:
            ice::Allocator* _allocator{ nullptr };
            CustomDeleterFunction* _deleter{ nullptr };
        };

    } // namespace detail

    //! \brief An allocator aware uniue_pointer type.
    template<typename T, typename D = detail::IceCustomDeleter<T>>
    using UniquePtr = std::unique_ptr<T, D>;

    namespace detail
    {

        template<class T>
        IceDefaultDeleter<T>::IceDefaultDeleter(ice::Allocator& alloc) noexcept
            : _allocator{ &alloc }
        { }

        template<class T>
        IceDefaultDeleter<T>::IceDefaultDeleter(IceDefaultDeleter&& other) noexcept
            : _allocator{ other._allocator }
        { }

        template<class T>
        auto IceDefaultDeleter<T>::operator=(IceDefaultDeleter&& other) noexcept -> IceDefaultDeleter&
        {
            if (this != &other)
            {
                _allocator = std::exchange(other._allocator, nullptr);
            }
            return *this;
        }

        template<class T>
        void IceDefaultDeleter<T>::operator()(T* object) noexcept
        {
            _allocator->destroy<T>(object);
        }

        template<class T>
        IceCustomDeleter<T>::IceCustomDeleter(
            ice::Allocator& alloc
        ) noexcept
            : _allocator{ &alloc }
            , _deleter{ &IceCustomDeleter::default_deleter_function }
        { }

        template<class T>
        IceCustomDeleter<T>::IceCustomDeleter(
            ice::Allocator& alloc,
            CustomDeleterFunction* func
        ) noexcept
            : _allocator{ &alloc }
            , _deleter{ func }
        { }

        template<class T>
        IceCustomDeleter<T>::IceCustomDeleter(IceCustomDeleter&& other) noexcept
            : _allocator{ other._allocator }
            , _deleter{ other._deleter }
        { }

        template<class T>
        auto IceCustomDeleter<T>::operator=(IceCustomDeleter&& other) noexcept -> IceCustomDeleter&
        {
            if (this != &other)
            {
                _allocator = std::exchange(other._allocator, nullptr);
                _deleter = std::exchange(other._deleter, nullptr);
            }
            return *this;
        }

        template<class T>
        void IceCustomDeleter<T>::operator()(T* object) noexcept
        {
            if (_deleter != nullptr)
            {
                _deleter(*_allocator, object);
            }
            else
            {
                _allocator->destroy(object);
            }
        }

        template<class T>
        void IceCustomDeleter<T>::default_deleter_function(
            ice::Allocator& alloc,
            T* object
        ) noexcept
        {
            alloc.destroy(object);
        }

    } // namespace detail

    template<class Result, class Type, class... Args>
    auto make_unique(ice::Allocator& alloc, Args&&... args) noexcept -> ice::UniquePtr<Result>
    {
        return ice::UniquePtr<Result> {
            alloc.make<Type>(ice::forward<Args>(args)...),
            detail::IceCustomDeleter<Result>{ alloc }
        };
    }

    template<typename Result>
    auto make_unique_null() noexcept -> ice::UniquePtr<Result>
    {
        return ice::UniquePtr<Result> {
            nullptr,
            detail::IceCustomDeleter<Result>{ ice::memory::null_allocator() }
        };
    }

} // namespace ice
