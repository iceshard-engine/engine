#include <memsys/memsys.h>
#include "dlmalloc/dlmalloc.h"

#include <algorithm>
#include <assert.h>
#include <atomic>

namespace memsys
{

// Header stored at the beginning of a memory allocation to indicate the
// size of the allocated data.
struct mem_header {
    uint32_t size;
};

// If we need to align the memory allocation we pad the header with this
// value after storing the size. That way we can find the pointer header
const uint32_t HEADER_PAD_VALUE = 0xffffffffu;

// Given a pointer to the header, returns a pointer to the data that follows it.
inline void* data_pointer(mem_header* header, uint32_t align) {
    void* ptr = header + 1;
    return utils::align_forward(ptr, align);
}

// Given a pointer to the data, returns a pointer to the header before it.
inline mem_header *header(void *data)
{
    static_assert(sizeof(mem_header) == sizeof(uint32_t), "Allocation header size is not valid! Cannot get a valid pointer!");
    auto* ptr = reinterpret_cast<uint32_t*>(data);
    while (ptr[-1] == HEADER_PAD_VALUE)
        --ptr;
    return reinterpret_cast<mem_header*>(ptr) - 1;
}

// Stores the size in the header and pads with HEADER_PAD_VALUE up to the
// data pointer.
inline void fill(mem_header* header, void* data, uint32_t size)
{
    static_assert(sizeof(mem_header) == sizeof(uint32_t), "Allocation header size is not valid! Cannot fill the memory properly!");
    header->size = size;
    auto* ptr = reinterpret_cast<uint32_t*>(header + 1);
    while (ptr < data)
        *ptr++ = HEADER_PAD_VALUE;
}

/// An allocator that uses the default system malloc(). Allocations are
/// padded so that we can store the size of each allocation and align them
/// to the desired alignment.
///
/// (Note: An OS-specific allocator that can do alignment and tracks size
/// does need this padding and can thus be more efficient than the
/// MallocAllocator.)
class dlmalloc_allocator : public allocator
{
public:
    dlmalloc_allocator() : m_TotalAllocated(0) { }
    virtual ~dlmalloc_allocator() override {
        // Check that we don't have any memory leaks when allocator is destroyed.
        assert(m_TotalAllocated == 0);
    }

    virtual void *allocate(uint32_t size, uint32_t align) override {
        const uint32_t ts = size_with_padding(size, align);
        auto* header = reinterpret_cast<mem_header*>(dlmalloc(ts));
        void* ptr = data_pointer(header, align);
        fill(header, ptr, ts);
        m_TotalAllocated += ts;
        return ptr;
    }

    virtual void deallocate(void* ptr) override {
        if (!ptr)
            return;

        mem_header* h = header(ptr);
        m_TotalAllocated -= h->size;
        dlfree(h);
    }

    virtual uint32_t allocated_size(void* ptr) override {
        return header(ptr)->size;
    }

    virtual uint32_t total_allocated() override {
        return m_TotalAllocated;
    }

private:
    std::atomic<uint32_t> m_TotalAllocated;

    // Returns the size to allocate from malloc() for a given size and align.
    static inline uint32_t size_with_padding(uint32_t size, uint32_t align) {
        return size + align + sizeof(mem_header);
    }
};

/// An allocator used to allocate temporary "scratch" memory. The allocator
/// uses a fixed size ring buffer to services the requests.
///
/// Memory is always always allocated linearly. An allocation pointer is
/// advanced through the buffer as memory is allocated and wraps around at
/// the end of the buffer. Similarly, a free pointer is advanced as memory
/// is freed.
///
/// It is important that the scratch allocator is only used for short-lived
/// memory allocations. A long lived allocator will lock the "free" pointer
/// and prevent the "allocate" pointer from proceeding past it, which means
/// the ring buffer can't be used.
///
/// If the ring buffer is exhausted, the scratch allocator will use its backing
/// allocator to allocate memory instead.
class scratch_allocator : public allocator
{
public:
    /// Creates a ScratchAllocator. The allocator will use the backing
    /// allocator to create the ring buffer and to service any requests
    /// that don't fit in the ring buffer.
    ///
    /// size specifies the size of the ring buffer.
    scratch_allocator(allocator &backing, uint32_t size) : m_Backing(backing) {
        m_Begin = reinterpret_cast<char*>(m_Backing.allocate(size));
        m_End = m_Begin + size;
        m_Allocate = m_Begin;
        m_Free = m_Begin;
    }

    virtual ~scratch_allocator() override {
        assert(m_Free == m_Allocate);
        m_Backing.deallocate(m_Begin);
    }

    bool in_use(void* ptr)
    {
        if (m_Free == m_Allocate)
            return false;
        if (m_Allocate > m_Free)
            return ptr >= m_Free && ptr < m_Allocate;
        return ptr >= m_Free || ptr < m_Allocate;
    }

    virtual void* allocate(uint32_t size, uint32_t align) override {
        assert(align % 4 == 0);
        size = ((size + 3) / 4) * 4;

        char* ptr = m_Allocate;
        auto* header = reinterpret_cast<mem_header*>(ptr);
        auto* data = reinterpret_cast<char*>(data_pointer(header, align));
        ptr = data + size;

        // Reached the end of the buffer, wrap around to the beginning.
        if (ptr > m_End) {
            header->size = static_cast<uint32_t>(m_End - reinterpret_cast<char*>(header)) | 0x80000000u;

            ptr = m_Begin;
            header = reinterpret_cast<mem_header*>(ptr);
            data = reinterpret_cast<char*>(data_pointer(header, align));
            ptr = data + size;
        }

        // If the buffer is exhausted use the backing allocator instead.
        if (in_use(ptr))
            return m_Backing.allocate(size, align);

        fill(header, data, static_cast<uint32_t>(ptr - reinterpret_cast<char*>(header)));
        m_Allocate = ptr;
        return data;
    }

    virtual void deallocate(void *p) override {
        if (!p)
            return;

        if (p < m_Begin || p >= m_End) {
            m_Backing.deallocate(p);
            return;
        }

        // Mark this slot as free
        mem_header* h = header(p);
        assert((h->size & 0x80000000u) == 0);
        h->size = h->size | 0x80000000u;

        // We don't need 'h' anymore.
        h = nullptr;

        // Advance the free pointer past all free slots.
        while (m_Free != m_Allocate) {
            h = reinterpret_cast<mem_header*>(m_Free);
            if ((h->size & 0x80000000u) == 0)
                break;

            m_Free += h->size & 0x7fffffffu;
            if (m_Free == m_End)
                m_Free = m_Begin;
        }
    }

    virtual uint32_t allocated_size(void *p) override {
        mem_header* h = header(p);
        return h->size - static_cast<uint32_t>(reinterpret_cast<char*>(p) - reinterpret_cast<char*>(h));
    }

    virtual uint32_t total_allocated() override {
        return static_cast<uint32_t>(m_End - m_Begin);
    }

private:
    // Start and end of the ring buffer.
    char *m_Begin, *m_End;

    // Pointers to where to allocate memory and where to free memory.
    char *m_Allocate, *m_Free;

    // The backing allocator
    allocator &m_Backing;
};

struct memory_globals {
    static constexpr uint32_t ALLOCATOR_MEMORY = sizeof(dlmalloc_allocator) + sizeof(scratch_allocator);
    char buffer[ALLOCATOR_MEMORY];

    dlmalloc_allocator* default_allocator;
    scratch_allocator* default_scratch_allocator;

    memory_globals() : default_allocator(nullptr), default_scratch_allocator(nullptr) {}
};

memory_globals g_MemoryGlobals;

namespace globals
{


void init(uint32_t temporary_memory) noexcept
{
    char* ptr = g_MemoryGlobals.buffer;
    g_MemoryGlobals.default_allocator = new (ptr) dlmalloc_allocator();
    ptr += sizeof(dlmalloc_allocator);
    g_MemoryGlobals.default_scratch_allocator = new (ptr) scratch_allocator(*g_MemoryGlobals.default_allocator, temporary_memory);
}

auto default_allocator() noexcept -> allocator&
{
    return *g_MemoryGlobals.default_allocator;
}

auto default_scratch_allocator() noexcept -> allocator&
{
    return *g_MemoryGlobals.default_scratch_allocator;
}

void shutdown() noexcept
{
    g_MemoryGlobals.default_scratch_allocator->~scratch_allocator();
    g_MemoryGlobals.default_allocator->~dlmalloc_allocator();
    g_MemoryGlobals = memory_globals();
}


} // namespace globals
} // namespace memsys
