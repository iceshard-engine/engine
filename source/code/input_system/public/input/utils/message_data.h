#pragma once
#include <core/allocator.hxx>
#include <core/cexpr/stringid.hxx>
#include <functional>

namespace mooned::io::message
{

struct Metadata
{
    using identifier_t = core::cexpr::stringid_type;
    using timestamp_t = uint32_t;

    identifier_t identifier;
    timestamp_t timestamp;
};

class Data
{
public:
    Data(core::allocator& alloc);
    ~Data();

    void clear();

    void push(Metadata meta, const void* ptr, int size);

    void for_each(std::function<void(Metadata, const void* data, int size)> func) const;

    int size() const;

protected:
    int available_space() const;
    void resize(int size);

private:
    core::allocator& _allocator;
    int _allocated;
    int _size;

    void* _data;
    void* _next;
};

}

