#pragma once
#include <core/base.hxx>
#include <core/pod/collections.hxx>

namespace core::pod
{

    //! \brief Contains functions used to modify Array<T> objects.
    namespace array
    {

        //! \brief Number of elements in the array.
        template<typename T>
        auto size(const Array<T>& a) noexcept -> uint32_t;

        //! \brief Checks if there are elements in the array.
        template<typename T>
        bool any(const Array<T>& a) noexcept;

        //! \brief Checks if the array is empty.
        template<typename T>
        bool empty(const Array<T>& a) noexcept;

        //! \brief Start iterator position.
        template<typename T>
        auto begin(Array<T>& a) noexcept -> T*;

        //! \brief Start iterator position.
        template<typename T>
        auto begin(const Array<T>& a) noexcept -> const T*;

        //! \brief End iterator position.
        template<typename T>
        auto end(Array<T>& a) noexcept -> T*;

        //! \brief End iterator position.
        template<typename T>
        auto end(const Array<T>& a) noexcept -> const T*;

        //! \brief First element of the array.
        //! \remarks Cannot be used on a empty array.
        template<typename T>
        auto front(Array<T>& a) noexcept -> T&;

        //! \brief First element of the array.
        //! \remarks Cannot be used on a empty array.
        template<typename T>
        auto front(const Array<T>& a) noexcept -> const T&;

        //! \brief Last element of the array.
        //! \remarks Cannot be used on a empty array.
        template<typename T>
        auto back(Array<T>& a) noexcept -> T&;

        //! \brief Last element of the array.
        //! \remarks Cannot be used on a empty array.
        template<typename T>
        auto back(const Array<T>& a) noexcept -> const T&;

        //! \brief Changes the size of the array.
        //! \remarks Does not reallocate memory unless necessary.
        template<typename T>
        void resize(Array<T>& a, uint32_t new_size) noexcept;

        //! \brief Removes all items in the array.
        //! \remarks Does not free memory.
        template<typename T>
        void clear(Array<T>& a) noexcept;

        //! \brief Reallocates the array to the specified capacity.
        template<typename T>
        void set_capacity(Array<T>& a, uint32_t new_capacity) noexcept;

        //! \brief Ensures the array has at least the specified capacity.
        template<typename T>
        void reserve(Array<T>& a, uint32_t new_capacity) noexcept;

        //! \brief Grows the array using a geometric progression formula.
        //!
        //! \details The amortized cost of push_back() is O(1). If a min_capacity is specified, the array will
        //!     grow to at least that capacity.
        template<typename T>
        void grow(Array<T>& a, uint32_t min_capacity = 0) noexcept;

        //! \brief Trims the array so that its capacity matches its size.
        template<typename T>
        void trim(Array<T>& a) noexcept;

        //! \brief Pushes the item to the end of the array.
        template<typename T, typename U = T>
        void push_back(Array<T>& a, U const& item) noexcept;

        //! \brief Pushes all items to the end of the array.
        template<typename T>
        void push_back(Array<T>& a, Array<T> const& items) noexcept;

        //! \brief Pops the last item from the array. The array cannot be empty.
        template<typename T>
        void pop_back(Array<T>& a) noexcept;

        template<typename T>
        void create_view(Array<T>& a, T* data, uint32_t count) noexcept;

        template<typename T>
        auto create_view(T* data, uint32_t count) noexcept -> Array<T>;

        template<typename T, uint32_t Size>
        auto create_view(T(&data)[Size]) noexcept -> Array<T>;

    } // namespace array

    template<typename T>
    auto begin(Array<T>& a) noexcept -> T*;

    template<typename T>
    auto begin(const Array<T>& a) noexcept -> const T*;

    template<typename T>
    auto end(Array<T>& a) noexcept -> T*;

    template<typename T>
    auto end(const Array<T>& a) noexcept -> const T*;

    template<typename T>
    void swap(Array<T>& lhs, Array<T>& rhs) noexcept;

} // namespace core::pod

#include "array.inl"
