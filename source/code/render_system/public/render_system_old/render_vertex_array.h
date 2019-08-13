#pragma once
#include <collections/pod/hash.h>
#include <kernel/utils/strong_typedef.h>

namespace mooned::render
{

//! Describes a single vertex array binding with its index and divisor
struct VertexArrayBinding
{
    int index;
    int divisor;
};

//! Describes a single vertex array binding attribute
struct VertexArrayAttribute
{
    //! Supported attribute types
    enum class Type : int32_t
    {
        FLOAT,
        UNSIGNED_BYTE,
    } type;

    uint32_t index;
    uint32_t offset;
    int32_t size;
    bool normalized;
};

//! Holds information about a single vertex array and it's bindings and attributes
class VertexArray
{
public:
    using Handle = mooned::strong_numeric_typedef<VertexArray, uint32_t>;

    VertexArray(mem::allocator& alloc);
    ~VertexArray();

    //! Adds another binding to the VA definition.
    //! \note If a valid handle exists while adding another binding, it will not update that handle.
    //! \note To update an existing handle, it needs to be released first.
    void add_binding(int binding, int divisor, VertexArrayAttribute* attribs, uint32_t count);

    template<size_t Count>
    void add_binding(int binding, int divisor, VertexArrayAttribute(&attributes)[Count])
    {
        add_binding(binding, divisor, &attributes[0], Count);
    }

    //! Returns the handle state
    //! \note Returning 'true' means the handle is in use and won't update when bindings are added!
    bool valid() const;

    //! Returns a handle to be used in a render command buffer.
    //! \note If a handle does not exist, it will create one.
    Handle get_handle();

    //! Releases the underlying handle, which makes it unusable in render command buffers.
    //! \note If the handle does not exist, nothing happens.
    void release_handle();

protected:
    //! Checks if the given binding has the given attribute
    bool has_attrib(int binding, const VertexArrayAttribute& attrib);

private:
    mem::allocator& _allocator;

    pod::Array<VertexArrayBinding> _bindings;
    pod::Hash<VertexArrayAttribute> _binding_attributes;

    Handle _handle;
};

} // namespace mooned::render
