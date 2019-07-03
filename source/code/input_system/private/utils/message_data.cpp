#include <iolib/utils/message_data.h>
#include <memsys/memsys.h>

#include <cassert>

namespace mooned::io::message::detail
{

static constexpr uint32_t DATA_ENTRY_ALIGNMENT = 8u;

struct DataEntry
{
    Metadata metadata;
    int32_t size;
};

static DataEntry* first_entry(void* ptr)
{
    return reinterpret_cast<DataEntry*>(mem::utils::align_forward(ptr, DATA_ENTRY_ALIGNMENT));
}

static DataEntry* next_entry(DataEntry* entry)
{
    auto* new_ptr = mem::utils::pointer_add(entry, sizeof(DataEntry) + entry->size);
    new_ptr = mem::utils::align_forward(new_ptr, DATA_ENTRY_ALIGNMENT);
    return reinterpret_cast<DataEntry*>(new_ptr);
}

static void* data_from_entry(DataEntry* entry)
{
    assert(entry && entry->size > 0);
    return mem::utils::pointer_add(entry, sizeof(DataEntry));
}

}

mooned::io::message::Data::Data(mem::allocator& alloc)
    : _allocator{ alloc }
    , _allocated{ 0 }
    , _size{ 0 }
    , _data{ nullptr }
    , _next{ nullptr }
{
    resize(1 KB);
}

mooned::io::message::Data::~Data()
{
    _allocator.deallocate(_data);
}

void mooned::io::message::Data::clear()
{
    _allocator.deallocate(_data);
    _allocated = 0;
    _size = 0;
    _data = nullptr;
    _next = nullptr;

    resize(1 KB);
}

void mooned::io::message::Data::push(Metadata meta, const void* ptr, int size)
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

void mooned::io::message::Data::for_each(std::function<void(Metadata, const void* data, int size)> func) const
{
    if (nullptr == _data || 0 == _size)
    {
        return;
    }

    auto* entry = detail::first_entry(_data);
    while (0llu != entry->metadata.identifier)
    {
        func(entry->metadata, detail::data_from_entry(entry), entry->size);

        entry = detail::next_entry(entry);
    }
}

int mooned::io::message::Data::size() const
{
    return _size;
}

int mooned::io::message::Data::available_space() const
{
    return mem::utils::pointer_distance(_next, mem::utils::pointer_add(_data, _allocated));
}

void mooned::io::message::Data::resize(int bytes)
{
    assert(_allocated < bytes);

    auto used = mem::utils::pointer_distance(_data, _next);
    void* new_data = _allocator.allocate(bytes, detail::DATA_ENTRY_ALIGNMENT);
    memset(new_data, 0, bytes);

    if (_data)
    {
        memcpy(new_data, _data, used);
        _allocator.deallocate(_data);
    }

    _data = new_data;
    _next = mem::utils::pointer_add(_data, used);
    _allocated = bytes;

    assert(detail::first_entry(_next) == _next);
}
