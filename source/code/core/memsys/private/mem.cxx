#include <ice/mem.hxx>
#include <ice/mem_data.hxx>
#include <ice/mem_memory.hxx>
#include <assert.h>

namespace ice
{

    auto alloc(ice::usize request) noexcept -> ice::AllocResult
    {
#if ISP_WINDOWS || ISP_UNIX
        return ice::AllocResult{
            .memory = malloc(request.value),
            .size = request,
            .alignment = ice::build::is_x64 ? ice::ualign::b_16 : ice::ualign::b_8,
        };
#else
        ICE_ASSERT_CORE(false);
        return nullptr;
#endif
    }

    void release(void* pointer) noexcept
    {
#if ISP_WINDOWS || ISP_UNIX
        free(pointer);
#else
        ICE_ASSERT_CORE(false);
#endif
    }

    auto alloc_aligned(ice::usize size, ice::ualign alignment) noexcept -> ice::AllocResult
    {
#if ISP_WINDOWS
        return ice::AllocResult{
            .memory = _aligned_malloc(size.value, static_cast<ice::u32>(alignment)),
            .size = size,
            .alignment = alignment,
        };
#elif ISP_UNIX
        return ice::AllocResult{
            .result = aligned_alloc(static_cast<ice::u32>(request.alignment), request.size.value),
            .size = request.size,
            .alignment = request.alignment,
        };
#else
        ICE_ASSERT_CORE(false);
        return nullptr;
#endif
    }

    void release_aligned(void* pointer) noexcept
    {
#if ISP_WINDOWS
        _aligned_free(pointer);
#elif ISP_UNIX
        free(AllocResult.result);
#else
        ICE_ASSERT_CORE(false);
        return nullptr;
#endif
    }

    auto memcpy(void* dest, void const* source, ice::usize size) noexcept -> void*
    {
        return std::memcpy(dest, source, size.value);
    }

    auto memcpy(ice::Memory memory, ice::Data data) noexcept -> ice::Memory
    {
        ICE_ASSERT_CORE(memory.alignment >= data.alignment);

        ice::usize const copy_size = ice::usize{ ice::min(memory.size.value, data.size.value) };
        ice::memcpy(memory.location, data.location, copy_size);
        return memory;
    }

} // namespace ice
