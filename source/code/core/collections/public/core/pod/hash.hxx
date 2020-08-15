#pragma once
#include <core/pod/collections.hxx>
#include <core/pod/array.hxx>

namespace core::pod
{

    //! \brief The hash functions store its data in a "list-in-an-array" where
    //!     indices are used instead of pointers.
    //!
    //! \details When items are removed, the array-list is repacked to always keep
    //!     it tightly ordered.
    namespace hash
    {

        //! \brief Checks for a key in the hash.
        template<typename T>
        bool has(Hash<T> const& h, uint64_t key) noexcept;

        //! \brief Returns the value stored for the specified key.
        //!
        //! \details Returns the default value if the key does not
        //!     exist in the hash.
        template<typename T>
        auto get(Hash<T> const& h, uint64_t key, T const& fallback_value) noexcept -> T const&;

        //! \brief Returns the value stored for the specified key.
        //!
        //! \details Returns the default value if the key does not
        //!     exist in the hash.
        template<typename T>
        auto get(Hash<T*> const& h, uint64_t key, nullptr_t) noexcept -> T*;

        //! \brief Sets the value for the key.
        template<typename T>
        void set(Hash<T> &h, uint64_t key, const T &value) noexcept;

        //! \brief Removes the key and associated value from the hash if it exists.
        template<typename T>
        void remove(Hash<T> &h, uint64_t key) noexcept;

        //! \brief Resizes the hash lookup table to the specified size.
        //! \remarks The table will grow automatically when 70% full.
        template<typename T>
        void reserve(Hash<T> &h, uint32_t size) noexcept;

        //! \brief Remove all elements from the hash.
        template<typename T>
        void clear(Hash<T> &h) noexcept;

        //! \brief Returns a pointer to the first entry in the hash table, can be used to
        //!     efficiently iterate over the elements (in random order).
        template<typename T>
        auto begin(Hash<T> const& h) noexcept -> typename Hash<T>::Entry const*;

        //! \brief Returns a pointer to the last entry in the hash table, can be used to
        //!     efficiently iterate over the elements (in random order).
        template<typename T>
        auto end(Hash<T> const& h) noexcept -> typename Hash<T>::Entry const*;

        //! \brief Returns a pointer to the first entry in the hash table, can be used to
        //!     efficiently iterate over the elements (in random order).
        template<typename T>
        auto begin(Hash<T>& h) noexcept -> typename Hash<T>::Entry*;

        //! \brief Returns a pointer to the last entry in the hash table, can be used to
        //!     efficiently iterate over the elements (in random order).
        template<typename T>
        auto end(Hash<T>& h) noexcept -> typename Hash<T>::Entry*;

    } // namespace hash

    //! \brief The hash functions store its data in a "list-in-an-array" where
    //!     indices are used instead of pointers.
    //!
    //! \details When items are removed, the array-list is repacked to always keep
    //!     it tightly ordered.
    namespace multi_hash
    {

        //! \brief Finds the first entry with the specified key.
        template<typename T>
        auto find_first(const Hash<T> &h, uint64_t key) noexcept -> const typename Hash<T>::Entry *;

        //! \brief Finds the next entry with the same key as e.
        template<typename T>
        auto find_next(const Hash<T> &h, const typename Hash<T>::Entry *e) noexcept -> const typename Hash<T>::Entry *;

        //! \brief Returns the number of entries for the key.
        template<typename T>
        auto count(const Hash<T> &h, uint64_t key) noexcept -> uint32_t;

        //! \brief Returns all the entries for the specified key.
        //! \remarks Use a StackAllocator for the array to avoid allocating memory.
        template<typename T>
        void get(const Hash<T> &h, uint64_t key, Array<T> &items) noexcept;

        //! \brief Inserts the value as an additional value for the key.
        template<typename T>
        void insert(Hash<T> &h, uint64_t key, const T &value) noexcept;

        //! \brief Removes the specified entry.
        template<typename T>
        void remove(Hash<T> &h, const typename Hash<T>::Entry *e) noexcept;

        //! \brief Removes all entries with the specified key.
        template<typename T>
        void remove_all(Hash<T> &h, uint64_t key) noexcept;

    } // namespace multi_hash

    //! \brief Returns a pointer to the first entry in the hash table, can be used to
    //!     efficiently iterate over the elements (in random order).
    template<typename T>
    auto begin(const Hash<T>& h) noexcept -> const typename Hash<T>::Entry*
    {
        return hash::begin(h);
    }

    //! \brief Returns a pointer to the last entry in the hash table, can be used to
    //!     efficiently iterate over the elements (in random order).
    template<typename T>
    auto end(const Hash<T> &h) noexcept -> const typename Hash<T>::Entry*
    {
        return hash::end(h);
    }

    template<typename T>
    auto begin(Hash<T>& h) noexcept -> typename Hash<T>::Entry*
    {
        return hash::begin(h);
    }

    template<typename T>
    auto end(Hash<T>& h) noexcept -> typename Hash<T>::Entry*
    {
        return hash::end(h);
    }

} // namespace core::pod

#include "hash.inl"
