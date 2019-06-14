#include <memsys/allocators/scratch_allocator.hxx>
#include <memsys/memsys.hxx>


namespace memsys
{


namespace
{
struct mem_header { uint32_t size; };

// If we need to align the memory allocation we pad the header with this
// value after storing the size. That way we can find the pointer header
const uint32_t HEADER_PAD_VALUE = 0xffffffffu;

// Given a pointer to the header, returns a pointer to the data that follows it.
inline void* data_pointer(mem_header* header, uint32_t align) noexcept
{
    void* ptr = header + 1;
    return utils::align_forward(ptr, align);
}

// Given a pointer to the data, returns a pointer to the header before it.
inline mem_header *header(void *data) noexcept
{
    static_assert(sizeof(mem_header) == sizeof(uint32_t), "Allocation header size is not valid! Cannot get a valid pointer!");
    auto* ptr = reinterpret_cast<uint32_t*>(data);
    while (ptr[-1] == HEADER_PAD_VALUE)
        --ptr;
    return reinterpret_cast<mem_header*>(ptr) - 1;
}

// Stores the size in the header and pads with HEADER_PAD_VALUE up to the
// data pointer.
inline void fill(mem_header* header, void* data, uint32_t size) noexcept
{
    static_assert(sizeof(mem_header) == sizeof(uint32_t), "Allocation header size is not valid! Cannot fill the memory properly!");
    header->size = size;
    auto* ptr = reinterpret_cast<uint32_t*>(header + 1);
    while (ptr < data)
        *ptr++ = HEADER_PAD_VALUE;
}
}

scratch_allocator::scratch_allocator(allocator &backing, uint32_t size) noexcept
    : _backing(backing)
{
    _begin = reinterpret_cast<char*>(_backing.allocate(size));
    _end = _begin + size;
    _allocate = _begin;
    _free = _begin;

    memset(_begin, 0, _end - _begin);
}

scratch_allocator::~scratch_allocator() noexcept
{
    assert(_free == _allocate);
    _backing.deallocate(_begin);
}

bool scratch_allocator::in_use(void* ptr) noexcept
{
    if (_free == _allocate)
        return false;
    if (_allocate > _free)
        return ptr >= _free && ptr < _allocate;
    return ptr >= _free || ptr < _allocate;
}

void* scratch_allocator::allocate(uint32_t size, uint32_t align) noexcept
{
    assert(align % 4 == 0);
    size = ((size + 3) / align) * align;

    char* ptr = _allocate;
    auto* header = reinterpret_cast<mem_header*>(ptr);
    auto* data = reinterpret_cast<char*>(data_pointer(header, align));
    ptr = data + size;

    // Reached the end of the buffer, wrap around to the beginning.
    if (ptr > _end) {
        header->size = static_cast<uint32_t>(_end - reinterpret_cast<char*>(header)) | 0x80000000u;

        ptr = _begin;
        header = reinterpret_cast<mem_header*>(ptr);
        data = reinterpret_cast<char*>(data_pointer(header, align));
        ptr = data + size;
    }

    // If the buffer is exhausted use the backing allocator instead.
    if (in_use(ptr))
        return _backing.allocate(size, align);

    fill(header, data, utils::pointer_distance(header, ptr));
    _allocate = ptr;
    return data;
}

void scratch_allocator::deallocate(void *p) noexcept
{
    if (!p)
        return;

    if (p < _begin || p >= _end) {
        _backing.deallocate(p);
        return;
    }

    // Mark this slot as free
    mem_header* h = header(p);
    assert((h->size & 0x80000000u) == 0);
    h->size = h->size | 0x80000000u;

    // We don't need 'h' anymore.
    h = nullptr;

    // Advance the free pointer past all free slots.
    while (_free != _allocate) {
        h = reinterpret_cast<mem_header*>(_free);
        if ((h->size & 0x80000000u) == 0)
            break;

        _free += (h->size & 0x7fffffffu);
        if (_free == _end)
            _free = _begin;
    }
}

uint32_t scratch_allocator::allocated_size(void *p) noexcept
{
    mem_header* h = header(p);
    return h->size - static_cast<uint32_t>(reinterpret_cast<char*>(p) - reinterpret_cast<char*>(h));
}

uint32_t scratch_allocator::total_allocated() noexcept
{
    int32_t distance = utils::pointer_distance(_free, _allocate);
    if (distance < 0)
    {
        distance = utils::pointer_distance(_begin, _end) + distance;
    }
    return distance;
}

allocator& scratch_allocator::backing_allocator() noexcept
{
    return _backing;
}


} // namespace memsys
