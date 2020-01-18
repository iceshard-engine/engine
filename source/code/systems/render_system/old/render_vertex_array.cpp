#include <renderlib/render_vertex_array.h>
#include <cassert>

mooned::render::VertexArray::VertexArray(mem::allocator& alloc)
    : _allocator{ alloc }
    , _bindings{ alloc }
    , _binding_attributes{ alloc }
    , _handle{ 0 }
{
}

mooned::render::VertexArray::~VertexArray()
{
    pod::array::clear(_bindings);
    pod::hash::clear(_binding_attributes);

    release_handle();
}

void mooned::render::VertexArray::add_binding(int binding, int divisor, VertexArrayAttribute* attribs, uint32_t count)
{
    auto* it = attribs;
    auto* const end = attribs + count;

    pod::array::push_back(_bindings, { binding, divisor });

    while (end != it)
    {
        assert(!has_attrib(binding, *it));
        pod::multi_hash::insert(_binding_attributes, binding, *it);
        it++;
    }
}

bool mooned::render::VertexArray::has_attrib(int binding, const VertexArrayAttribute& attrib)
{
    bool contains = false;
    auto entry = pod::multi_hash::find_first(_binding_attributes, binding);

    while (nullptr != entry && !contains)
    {
        contains = entry->value.index == attrib.index;
        entry = pod::multi_hash::find_next(_binding_attributes, entry);
    }

    return contains;
}
