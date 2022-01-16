#pragma once
#include <ice/pod/collections.hxx>
#include <ice/pod/array.hxx>

namespace ice::pod
{

    namespace hash
    {

        template<typename T>
        inline void reserve(ice::pod::Hash<T>& hsh, uint32_t size) noexcept;

        template<typename T>
        inline void clear(ice::pod::Hash<T>& hsh) noexcept;

        template<typename T>
        inline void set(ice::pod::Hash<T>& hsh, uint64_t key, T const& value) noexcept;

        template<typename T>
        inline void remove(ice::pod::Hash<T>& hsh, uint64_t key) noexcept;


        template<typename T>
        inline bool empty(ice::pod::Hash<T> const& hsh) noexcept;

        template<typename T>
        inline bool has(ice::pod::Hash<T> const& hsh, uint64_t key) noexcept;

        template<typename T>
        inline auto get(ice::pod::Hash<T> const& hsh, uint64_t key, T const& fallback_value) noexcept -> T const&;

        template<typename T>
        inline auto get(ice::pod::Hash<T*> const& hsh, uint64_t key, std::nullptr_t) noexcept -> T*;

        template<typename T>
        inline auto begin(ice::pod::Hash<T> const& hsh) noexcept -> typename ice::pod::Hash<T>::ConstIterator;

        template<typename T>
        inline auto end(ice::pod::Hash<T> const& hsh) noexcept -> typename ice::pod::Hash<T>::ConstIterator;

    } // namespace hash

    namespace multi_hash
    {

        template<typename T>
        inline void insert(ice::pod::Hash<T>& hsh, uint64_t key, T const& value) noexcept;

        template<typename T>
        inline void remove(ice::pod::Hash<T>& hsh, typename ice::pod::Hash<T>::ConstIterator entry) noexcept;

        template<typename T>
        inline void remove_all(ice::pod::Hash<T>& hsh, uint64_t key) noexcept;


        template<typename T>
        inline auto count(ice::pod::Hash<T> const& hsh, uint64_t key) noexcept -> uint32_t;

        template<typename T>
        inline void get(ice::pod::Hash<T> const& hsh, uint64_t key, ice::pod::Array<T>& items) noexcept;

        template<typename T>
        inline auto find_first(ice::pod::Hash<T> const& hsh, uint64_t key) noexcept -> typename ice::pod::Hash<T>::ConstIterator;

        template<typename T>
        inline auto find_next(ice::pod::Hash<T> const& hsh, typename ice::pod::Hash<T>::ConstIterator entry) noexcept -> typename ice::pod::Hash<T>::ConstIterator;

    } // namespace multi_hash

    template<typename T>
    inline Hash<T>::Hash(ice::Allocator& alloc) noexcept
        : _data{ alloc }
        , _hash{ alloc }
    { }

    template<typename T>
    inline Hash<T>::Hash(Hash&& other) noexcept
        : _data{ ice::move(other._data) }
        , _hash{ ice::move(other._hash) }
    { }

    template<typename T>
    auto Hash<T>::operator=(Hash&& other) noexcept -> Hash&
    {
        if (this != &other)
        {
            _data = ice::move(other._data);
            _hash = ice::move(other._hash);
        }
        return *this;
    }

    namespace detail::hash
    {

        static constexpr float Constant_MaxLoadFactor = 0.7f;

        static constexpr uint32_t Constant_EndOfList = 0xffffffffu;

        struct FindResult
        {
            uint32_t hash_i;
            uint32_t data_prev;
            uint32_t data_i;
        };

        template<typename T>
        inline auto add_entry(ice::pod::Hash<T>& hsh, uint64_t key) noexcept -> uint32_t
        {
            typename ice::pod::Hash<T>::Entry entry{
                .key = key,
                .next = Constant_EndOfList,
            };

            uint32_t ei = ice::pod::array::size(hsh._data);
            ice::pod::array::push_back(hsh._data, entry);
            return ei;
        }

        template<typename T>
        inline auto find(ice::pod::Hash<T> const& hsh, uint64_t key) noexcept -> FindResult
        {
            FindResult fr{
                .hash_i = Constant_EndOfList,
                .data_prev = Constant_EndOfList,
                .data_i = Constant_EndOfList,
            };

            if (ice::pod::array::size(hsh._hash) == 0)
            {
                return fr;
            }

            fr.hash_i = key % ice::pod::array::size(hsh._hash);
            fr.data_i = hsh._hash[fr.hash_i];

            while (fr.data_i != Constant_EndOfList)
            {
                if (hsh._data[fr.data_i].key == key)
                {
                    return fr;
                }

                fr.data_prev = fr.data_i;
                fr.data_i = hsh._data[fr.data_i].next;
            }
            return fr;
        }

        template<typename T>
        inline auto find(ice::pod::Hash<T> const& hsh, typename ice::pod::Hash<T>::ConstIterator it) noexcept -> FindResult
        {
            FindResult fr{
                .hash_i = Constant_EndOfList,
                .data_prev = Constant_EndOfList,
                .data_i = Constant_EndOfList,
            };

            if (array::size(hsh._hash) == 0)
            {
                return fr;
            }

            fr.hash_i = it->key % array::size(hsh._hash);
            fr.data_i = hsh._hash[fr.hash_i];
            while (fr.data_i != Constant_EndOfList)
            {
                if (&hsh._data[fr.data_i] == it)
                {
                    return fr;
                }
                fr.data_prev = fr.data_i;
                fr.data_i = hsh._data[fr.data_i].next;
            }
            return fr;
        }

        template<typename T>
        inline auto find_or_fail(ice::pod::Hash<T> const& hsh, uint64_t key) noexcept -> uint32_t
        {
            return detail::hash::find(hsh, key).data_i;
        }

        template<typename T>
        inline auto find_or_make(ice::pod::Hash<T>& hsh, uint64_t key) noexcept -> uint32_t
        {
            FindResult const fr = detail::hash::find(hsh, key);

            if (fr.data_i != Constant_EndOfList)
            {
                return fr.data_i;
            }

            uint32_t i = detail::hash::add_entry(hsh, key);
            if (fr.data_prev == Constant_EndOfList)
            {
                hsh._hash[fr.hash_i] = i;
            }
            else
            {
                hsh._data[fr.data_prev].next = i;
            }

            return i;
        }

        template<typename T>
        inline void erase(ice::pod::Hash<T>& hsh, FindResult const& fr) noexcept
        {
            if (fr.data_prev == Constant_EndOfList)
            {
                hsh._hash[fr.hash_i] = hsh._data[fr.data_i].next;
            }
            else
            {
                hsh._data[fr.data_prev].next = hsh._data[fr.data_i].next;
            }

            if (fr.data_i == ice::pod::array::size(hsh._data) - 1)
            {
                ice::pod::array::pop_back(hsh._data);
                return;
            }

            hsh._data[fr.data_i] = ice::pod::array::back(hsh._data);
            FindResult last = detail::hash::find(hsh, hsh._data[fr.data_i].key);

            if (last.data_prev != Constant_EndOfList)
            {
                T const saved_value = hsh._data[last.data_prev].value;
                ice::u64 const saved_key = hsh._data[last.data_prev].key;

                new (hsh._data._data + last.data_prev) typename ice::pod::Hash<T>::Entry{
                    .key = saved_key,
                    .next = fr.data_i,
                    .value = saved_value,
                };
            }
            else
            {
                hsh._hash[last.hash_i] = fr.data_i;
            }

            ice::pod::array::pop_back(hsh._data);
        }

        template<typename T>
        inline auto make(ice::pod::Hash<T>& hsh, uint64_t key) noexcept -> uint32_t
        {
            FindResult const fr = detail::hash::find(hsh, key);
            uint32_t const index = detail::hash::add_entry(hsh, key);

            if (fr.data_prev == Constant_EndOfList)
            {
                hsh._hash[fr.hash_i] = index;
            }
            else
            {
                hsh._data[fr.data_prev].next = index;
            }

            hsh._data[index].next = fr.data_i;
            return index;
        }

        template<typename T>
        inline void find_and_erase(ice::pod::Hash<T>& h, uint64_t key) noexcept
        {
            FindResult const fr = detail::hash::find(h, key);
            if (fr.data_i != Constant_EndOfList)
            {
                detail::hash::erase(h, fr);
            }
        }

        template<typename T>
        inline void rehash(ice::pod::Hash<T>& hsh, uint32_t new_size) noexcept
        {
            ice::pod::Hash<T> nh{ *hsh._hash._allocator };
            ice::pod::array::resize(nh._hash, new_size);
            ice::pod::array::reserve(nh._data, static_cast<ice::u32>(ice::pod::array::size(nh._hash) * (Constant_MaxLoadFactor + 0.05f)));
            for (uint32_t i = 0; i < new_size; ++i)
            {
                nh._hash[i] = Constant_EndOfList;
            }

            for (uint32_t i = 0; i < ice::pod::array::size(hsh._data); ++i)
            {
                typename Hash<T>::Entry const& entry = hsh._data[i];
                ice::pod::multi_hash::insert(nh, entry.key, entry.value);
            }

            hsh = ice::move(nh);
        }

        template<typename T>
        inline bool full(ice::pod::Hash<T> const& hsh) noexcept
        {
            return ice::pod::array::size(hsh._data) >= ice::pod::array::size(hsh._hash) * Constant_MaxLoadFactor;
        }

        template<typename T>
        inline void grow(ice::pod::Hash<T>& h) noexcept
        {
            uint32_t const new_size = ice::pod::array::size(h._data) * 2 + 10;
            detail::hash::rehash(h, new_size);
        }

    } // namespace ice::pod::detail::hash

    namespace hash
    {

        template<typename T>
        inline void reserve(ice::pod::Hash<T>& hsh, uint32_t size) noexcept
        {
            detail::hash::rehash(hsh, size);
        }

        template<typename T>
        inline void clear(Hash<T>& hsh) noexcept
        {
            ice::pod::array::clear(hsh._data);
            ice::pod::array::clear(hsh._hash);
        }

        template<typename T>
        inline void set(ice::pod::Hash<T>& hsh, uint64_t key, T const& value) noexcept
        {
            if (ice::pod::array::size(hsh._hash) == 0)
            {
                detail::hash::grow(hsh);
            }

            uint32_t const index = detail::hash::find_or_make(hsh, key);
            hsh._data[index].value = value;
            if (detail::hash::full(hsh))
            {
                detail::hash::grow(hsh);
            }
        }

        template<typename T>
        inline void remove(ice::pod::Hash<T>& hsh, uint64_t key) noexcept
        {
            detail::hash::find_and_erase(hsh, key);
        }


        template<typename T>
        inline bool empty(ice::pod::Hash<T> const& hsh) noexcept
        {
            return ice::pod::array::empty(hsh._data);
        }

        template<typename T>
        inline bool has(ice::pod::Hash<T> const& hsh, uint64_t key) noexcept
        {
            return detail::hash::find_or_fail(hsh, key) != detail::hash::Constant_EndOfList;
        }

        template<typename T>
        inline auto get(ice::pod::Hash<T> const& hsh, uint64_t key, T const& fallback_value) noexcept -> T const&
        {
            uint32_t const index = detail::hash::find_or_fail(hsh, key);
            return index == detail::hash::Constant_EndOfList ? fallback_value : hsh._data[index].value;
        }

        template<typename T>
        inline auto get(ice::pod::Hash<T*> const& hsh, uint64_t key, std::nullptr_t) noexcept -> T*
        {
            uint32_t const index = detail::hash::find_or_fail(hsh, key);
            return index == detail::hash::Constant_EndOfList ? nullptr : hsh._data[index].value;
        }

        template<typename T>
        inline auto begin(ice::pod::Hash<T> const& hsh) noexcept -> typename ice::pod::Hash<T>::ConstIterator
        {
            return ice::pod::array::begin(hsh._data);
        }

        template<typename T>
        inline auto end(ice::pod::Hash<T> const& hsh) noexcept -> typename ice::pod::Hash<T>::ConstIterator
        {
            return ice::pod::array::end(hsh._data);
        }

    } // namespace hash

    namespace multi_hash
    {

        template<typename T>
        inline void insert(ice::pod::Hash<T>& hsh, uint64_t key, T const& value) noexcept
        {
            if (ice::pod::array::size(hsh._hash) == 0)
            {
                detail::hash::grow(hsh);
            }

            uint32_t const index = detail::hash::make(hsh, key);
            hsh._data[index].value = value;
            if (detail::hash::full(hsh))
            {
                detail::hash::grow(hsh);
            }
        }

        template<typename T>
        inline void remove(ice::pod::Hash<T>& hsh, typename ice::pod::Hash<T>::ConstIterator it) noexcept
        {
            detail::hash::FindResult const fr = detail::hash::find(hsh, it);
            if (fr.data_i != detail::hash::Constant_EndOfList)
            {
                detail::hash::erase(hsh, fr);
            }
        }

        template<typename T>
        inline void remove_all(ice::pod::Hash<T>& hsh, uint64_t key) noexcept
        {
            while (ice::pod::hash::has(hsh, key))
            {
                ice::pod::hash::remove(hsh, key);
            }
        }

        template<typename T>
        inline auto count(ice::pod::Hash<T> const& hsh, uint64_t key) noexcept -> uint32_t
        {
            uint32_t result = 0;
            typename ice::pod::Hash<T>::ConstIterator it = ice::pod::multi_hash::find_first(hsh, key);
            while (it != nullptr)
            {
                result += 1;
                it = ice::pod::multi_hash::find_next(hsh, it);
            }
            return result;
        }

        template<typename T>
        inline void get(ice::pod::Hash<T> const& hsh, uint64_t key, ice::pod::Array<T>& items) noexcept
        {
            typename ice::pod::Hash<T>::ConstIterator it = ice::pod::multi_hash::find_first(hsh, key);
            while (it != nullptr)
            {
                ice::pod::array::push_back(items, it->value);
                it = ice::pod::multi_hash::find_next(hsh, it);
            }
        }

        template<typename T>
        inline auto find_first(ice::pod::Hash<T> const& hsh, uint64_t key) noexcept -> typename ice::pod::Hash<T>::ConstIterator
        {
            uint32_t const index = detail::hash::find_or_fail(hsh, key);
            return index == detail::hash::Constant_EndOfList ? nullptr : &hsh._data[index];
        }

        template<typename T>
        inline auto find_next(ice::pod::Hash<T> const& hsh, typename ice::pod::Hash<T>::ConstIterator it) noexcept -> typename ice::pod::Hash<T>::ConstIterator
        {
            uint32_t index = it->next;
            while (index != detail::hash::Constant_EndOfList)
            {
                if (hsh._data[index].key == it->key)
                {
                    return &hsh._data[index];
                }
                index = hsh._data[index].next;
            }
            return nullptr;
        }

    } // namespace multi_hash

    using hash::begin;
    using hash::end;

} // namespace ice::pod
