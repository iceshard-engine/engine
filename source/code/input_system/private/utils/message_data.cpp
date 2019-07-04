#include <input/utils/message_data.h>
#include <core/memory.hxx>

#include <cassert>

namespace input::message::detail
{

static constexpr uint32_t DATA_ENTRY_ALIGNMENT = 8u;

struct DataEntry
{
    Metadata metadata;
    int32_t size;
};

static DataEntry* first_entry(void* ptr)
{
    return reinterpret_cast<DataEntry*>(core::memory::utils::align_forward(ptr, DATA_ENTRY_ALIGNMENT));
}

static DataEntry* next_entry(DataEntry* entry)
{
    auto* new_ptr = core::memory::utils::pointer_add(entry, sizeof(DataEntry) + entry->size);
    new_ptr = core::memory::utils::align_forward(new_ptr, DATA_ENTRY_ALIGNMENT);
    return reinterpret_cast<DataEntry*>(new_ptr);
}

static void* data_from_entry(DataEntry* entry)
{
    assert(entry && entry->size > 0);
    return core::memory::utils::pointer_add(entry, sizeof(DataEntry));
}

}

input::message::Data::Data(core::allocator& alloc)
    : _allocator{ alloc }
    , _allocated{ 0 }
    , _size{ 0 }
    , _data{ nullptr }
    , _next{ nullptr }
{
    resize(1 * 1024 /* 1 KB */);
}

input::message::Data::~Data()
{
    _allocator.deallocate(_data);
}

void input::message::Data::clear()
{
    _allocator.deallocate(_data);
    _allocated = 0;
    _size = 0;
    _data = nullptr;
    _next = nullptr;

    resize(1 * 1024 /* 1 KB */);
}

void input::message::Data::push(Metadata meta, const void* ptr, int size)
{
    while (available_space() < size)
    {
        resize(static_cast<int>(_allocated * 1.5 + 0.5));
    }

    auto* entry = detail::first_entry(_next);
    entry->metadata = meta;
    entry->size = size;

    if (size > 0)
    {
        auto* entry_data = detail::data_from_entry(entry);
        memcpy(entry_data, ptr, size);
    }

    _next = detail::next_entry(entry);
    _size += 1;
}

void input::message::Data::for_each(std::function<void(Metadata, const void* data, int size)> func) const
{
    if (nullptr == _data || 0 == _size)
    {
        return;
    }

    auto* entry = detail::first_entry(_data);
    while (0llu != static_cast<uint64_t>(entry->metadata.identifier.hash_value))
    {
        func(entry->metadata, detail::data_from_entry(entry), entry->size);

        entry = detail::next_entry(entry);
    }
}

int input::message::Data::size() const
{
    return _size;
}

int input::message::Data::available_space() const
{
    return core::memory::utils::pointer_distance(_next, core::memory::utils::pointer_add(_data, _allocated));
}

void input::message::Data::resize(int bytes)
{
    assert(_allocated < bytes);

    auto used = core::memory::utils::pointer_distance(_data, _next);
    void* new_data = _allocator.allocate(bytes, detail::DATA_ENTRY_ALIGNMENT);
    memset(new_data, 0, bytes);

    if (_data)
    {
        memcpy(new_data, _data, used);
        _allocator.deallocate(_data);
    }

    _data = new_data;
    _next = core::memory::utils::pointer_add(_data, used);
    _allocated = bytes;

    assert(detail::first_entry(_next) == _next);
}
