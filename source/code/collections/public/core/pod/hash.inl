
// core::pod::Hash members
//////////////////////////////////////////////////////////////////////////


template <typename T>
Hash<T>::Hash(core::allocator &alloc) noexcept
    : _hash{ alloc }
    , _data{ alloc }
{}


// core::pod::Hash miscelanous functions
//////////////////////////////////////////////////////////////////////////


template<typename T>
auto begin(const Hash<T> &h) noexcept -> const typename Hash<T>::Entry*
{
    return array::begin(h._data);
}

template<typename T>
auto end(const Hash<T> &h) noexcept -> const typename Hash<T>::Entry*
{
    return array::end(h._data);
}

template<typename T>
void swap(Hash<T>& lhs, Hash<T>& rhs) noexcept
{
    std::swap(lhs._data, rhs._data);
    std::swap(lhs._hash, rhs._hash);
}


// core::pod::Hash internal functions
//////////////////////////////////////////////////////////////////////////


namespace hash_internal
{

    static constexpr uint32_t END_OF_LIST = 0xffffffffu;

    struct FindResult
    {
        uint32_t hash_i;
        uint32_t data_prev;
        uint32_t data_i;
    };

    template<typename T>
    auto add_entry(Hash<T> &h, uint64_t key) noexcept -> uint32_t
    {
        typename Hash<T>::Entry e;
        e.key = key;
        e.next = END_OF_LIST;
        uint32_t ei = array::size(h._data);
        array::push_back(h._data, e);
        return ei;
    }

    template<typename T>
    auto find(const Hash<T> &h, uint64_t key) noexcept -> FindResult
    {
        FindResult fr;
        fr.hash_i = END_OF_LIST;
        fr.data_prev = END_OF_LIST;
        fr.data_i = END_OF_LIST;

        if (array::size(h._hash) == 0)
        {
            return fr;
        }

        fr.hash_i = key % array::size(h._hash);
        fr.data_i = h._hash[fr.hash_i];
        while (fr.data_i != END_OF_LIST)
        {
            if (h._data[fr.data_i].key == key)
            {
                return fr;
            }

            fr.data_prev = fr.data_i;
            fr.data_i = h._data[fr.data_i].next;
        }
        return fr;
    }

    template<typename T>
    auto find(const Hash<T> &h, const typename Hash<T>::Entry *e) noexcept -> FindResult
    {
        FindResult fr;
        fr.hash_i = END_OF_LIST;
        fr.data_prev = END_OF_LIST;
        fr.data_i = END_OF_LIST;

        if (array::size(h._hash) == 0)
        {
            return fr;
        }

        fr.hash_i = e->key % array::size(h._hash);
        fr.data_i = h._hash[fr.hash_i];
        while (fr.data_i != END_OF_LIST)
        {
            if (&h._data[fr.data_i] == e)
            {
                return fr;
            }
            fr.data_prev = fr.data_i;
            fr.data_i = h._data[fr.data_i].next;
        }
        return fr;
    }

    template<typename T>
    auto find_or_fail(const Hash<T> &h, uint64_t key) noexcept -> uint32_t
    {
        return find(h, key).data_i;
    }

    template<typename T>
    auto find_or_make(Hash<T> &h, uint64_t key) noexcept -> uint32_t
    {
        const FindResult fr = find(h, key);
        if (fr.data_i != END_OF_LIST)
        {
            return fr.data_i;
        }

        uint32_t i = add_entry(h, key);
        if (fr.data_prev == END_OF_LIST)
        {
            h._hash[fr.hash_i] = i;
        }
        else
        {
            h._data[fr.data_prev].next = i;
        }

        return i;
    }

    template<typename T>
    void erase(Hash<T> &h, const FindResult &fr) noexcept
    {
        if (fr.data_prev == END_OF_LIST)
        {
            h._hash[fr.hash_i] = h._data[fr.data_i].next;
        }
        else
        {
            h._data[fr.data_prev].next = h._data[fr.data_i].next;
        }

        if (fr.data_i == array::size(h._data) - 1)
        {
            array::pop_back(h._data);
            return;
        }

        h._data[fr.data_i] = h._data[array::size(h._data) - 1];
        FindResult last = find(h, h._data[fr.data_i].key);

        if (last.data_prev != END_OF_LIST)
        {
            h._data[last.data_prev].next = fr.data_i;
        }
        else
        {
            h._hash[last.hash_i] = fr.data_i;
        }

        array::pop_back(h._data);
    }

    template<typename T>
    auto make(Hash<T> &h, uint64_t key) noexcept -> uint32_t
    {
        const FindResult fr = find(h, key);
        const uint32_t i = add_entry(h, key);

        if (fr.data_prev == END_OF_LIST)
        {
            h._hash[fr.hash_i] = i;
        }
        else
        {
            h._data[fr.data_prev].next = i;
        }

        h._data[i].next = fr.data_i;
        return i;
    }

    template<typename T>
    void find_and_erase(Hash<T> &h, uint64_t key) noexcept
    {
        const FindResult fr = find(h, key);
        if (fr.data_i != END_OF_LIST)
        {
            erase(h, fr);
        }
    }

    template<typename T>
    void rehash(Hash<T> &h, uint32_t new_size) noexcept
    {
        Hash<T> nh(*h._hash._allocator);
        array::resize(nh._hash, new_size);
        array::reserve(nh._data, array::size(h._data));
        for (uint32_t i = 0; i < new_size; ++i)
        {
            nh._hash[i] = END_OF_LIST;
        }

        for (uint32_t i = 0; i < array::size(h._data); ++i)
        {
            const typename Hash<T>::Entry &e = h._data[i];
            multi_hash::insert(nh, e.key, e.value);
        }

        Hash<T> empty(*h._hash._allocator);
        h.~Hash<T>();
        memcpy(&h, &nh, sizeof(Hash<T>));
        memcpy(&nh, &empty, sizeof(Hash<T>));
    }

    template<typename T>
    bool full(const Hash<T> &h) noexcept
    {
        const float max_load_factor = 0.7f;
        return array::size(h._data) >= array::size(h._hash) * max_load_factor;
    }

    template<typename T>
    void grow(Hash<T> &h) noexcept
    {
        const uint32_t new_size = array::size(h._data) * 2 + 10;
        rehash(h, new_size);
    }

} // namespace hash_internal


// core::pod::hash free functions
//////////////////////////////////////////////////////////////////////////


template<typename T>
bool hash::has(const Hash<T> &h, uint64_t key) noexcept
{
    return hash_internal::find_or_fail(h, key) != hash_internal::END_OF_LIST;
}

template<typename T>
auto hash::get(const Hash<T> &h, uint64_t key, const T &deffault) noexcept -> const T&
{
    const uint32_t i = hash_internal::find_or_fail(h, key);
    return i == hash_internal::END_OF_LIST ? deffault : h._data[i].value;
}

template<typename T>
void hash::set(Hash<T> &h, uint64_t key, const T &value) noexcept
{
    if (array::size(h._hash) == 0)
    {
        hash_internal::grow(h);
    }

    const uint32_t i = hash_internal::find_or_make(h, key);
    h._data[i].value = value;
    if (hash_internal::full(h))
    {
        hash_internal::grow(h);
    }
}

template<typename T>
void hash::remove(Hash<T> &h, uint64_t key) noexcept
{
    hash_internal::find_and_erase(h, key);
}

template<typename T>
void hash::reserve(Hash<T> &h, uint32_t size) noexcept
{
    hash_internal::rehash(h, size);
}

template<typename T>
void hash::clear(Hash<T> &h) noexcept
{
    array::clear(h._data);
    array::clear(h._hash);
}

template<typename T>
auto hash::begin(const Hash<T> &h) noexcept -> const typename Hash<T>::Entry*
{
    return array::begin(h._data);
}

template<typename T>
auto hash::end(const Hash<T> &h) noexcept -> const typename Hash<T>::Entry*
{
    return array::end(h._data);
}


// core::pod::multi_hash free functions
//////////////////////////////////////////////////////////////////////////


template<typename T>
auto multi_hash::find_first(const Hash<T> &h, uint64_t key) noexcept -> const typename Hash<T>::Entry*
{
    const uint32_t i = hash_internal::find_or_fail(h, key);
    return i == hash_internal::END_OF_LIST ? 0 : &h._data[i];
}

template<typename T>
auto multi_hash::find_next(const Hash<T> &h, const typename Hash<T>::Entry *e) noexcept -> const typename Hash<T>::Entry*
{
    uint32_t i = e->next;
    while (i != hash_internal::END_OF_LIST) {
        if (h._data[i].key == e->key)
            return &h._data[i];
        i = h._data[i].next;
    }
    return nullptr;
}

template<typename T>
auto multi_hash::count(const Hash<T> &h, uint64_t key) noexcept -> uint32_t
{
    uint32_t i = 0;
    const typename Hash<T>::Entry *e = find_first(h, key);
    while (e) {
        ++i;
        e = find_next(h, e);
    }
    return i;
}

template<typename T>
void multi_hash::get(const Hash<T> &h, uint64_t key, Array<T> &items) noexcept
{
    const typename Hash<T>::Entry *e = find_first(h, key);
    while (e) {
        array::push_back(items, e->value);
        e = find_next(h, e);
    }
}

template<typename T>
void multi_hash::insert(Hash<T> &h, uint64_t key, const T &value) noexcept
{
    if (array::size(h._hash) == 0)
    {
        hash_internal::grow(h);
    }

    const uint32_t i = hash_internal::make(h, key);
    h._data[i].value = value;
    if (hash_internal::full(h))
    {
        hash_internal::grow(h);
    }
}

template<typename T>
void multi_hash::remove(Hash<T> &h, const typename Hash<T>::Entry *e) noexcept
{
    const hash_internal::FindResult fr = hash_internal::find(h, e);
    if (fr.data_i != hash_internal::END_OF_LIST)
    {
        hash_internal::erase(h, fr);
    }
}

template<typename T>
void multi_hash::remove_all(Hash<T> &h, uint64_t key) noexcept
{
    while (hash::has(h, key))
    {
        hash::remove(h, key);
    }
}
