#pragma once
#include <memsys/allocator.h>
#include <kernel/compiletime/stringid.h>
#include <functional>

namespace mooned::io::message
{

struct Metadata
{
    using identifier_t = stringid_hash_t;
    using timestamp_t = uint32_t;

    identifier_t identifier;
    timestamp_t timestamp;
};

class Data
{
public:
    Data(mem::allocator& alloc);
    ~Data();

    void clear();

    void push(Metadata meta, const void* ptr, int size);

    void for_each(std::function<void(Metadata, const void* data, int size)> func) const;

    int size() const;

protected:
    int available_space() const;
    void resize(int size);

private:
    mem::allocator& _allocator;
    int _allocated;
    int _size;

    void* _data;
    void* _next;
};

}

