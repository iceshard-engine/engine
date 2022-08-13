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
            .result = malloc(request.value),
            .size = request,
            .alignment = ice::build::is_x64 ? ice::ualign::b_16 : ice::ualign::b_8,
        };
#else
        assert(false);
        return nullptr;
#endif
    }

    void release(ice::Memory memory) noexcept
    {
#if ISP_WINDOWS || ISP_UNIX
        assert(memory.alignment == (ice::build::is_x64 ? ice::ualign::b_16 : ice::ualign::b_8));
        free(memory.location);
#else
        assert(false);
#endif
    }

    auto alloc_aligned(ice::AllocRequest request) noexcept -> ice::AllocResult
    {
#if ISP_WINDOWS
        return ice::AllocResult{
            .result = _aligned_malloc(request.size.value, static_cast<ice::u32>(request.alignment)),
            .size = request.size,
            .alignment = request.alignment,
        };
#elif ISP_UNIX
        return ice::AllocResult{
            .result = aligned_alloc(static_cast<ice::u32>(request.alignment), request.size.value),
            .size = request.size,
            .alignment = request.alignment,
        };
#else
        assert(false);
        return nullptr;
#endif
    }

    void release_aligned(ice::Memory AllocResult) noexcept
    {
#if ISP_WINDOWS
        _aligned_free(AllocResult.location);
#elif ISP_UNIX
        free(AllocResult.result);
#else
        assert(false);
        return nullptr;
#endif
    }

    auto memcpy(void* dest, void const* source, ice::usize size) noexcept -> void*
    {
        return std::memcpy(dest, source, size.value);
    }

    auto memcpy(ice::Memory AllocResult, ice::Data data) noexcept -> ice::Memory
    {
        // Assert: (alignment)
        ice::usize const copy_size = ice::usize{ ice::min(AllocResult.size.value, data.size.value) };
        void* const result = ice::memcpy(AllocResult.location, data.location, copy_size);

        return Memory{
            .location = result,
            .size = ice::usize{ AllocResult.size.value - copy_size.value },
            .alignment = ice::ualign::b_1,
        };
    }

    // Additional overloads
    void release(void* pointer) noexcept
    {
        release(
            ice::AllocResult{ .result = pointer, .alignment = ice::ualign::b_default }
        );
    }

    void release_aligned(void* pointer, ice::ualign alignment) noexcept
    {
        release_aligned(
            ice::AllocResult{ .result = pointer, .alignment = alignment }
        );
    }

} // namespace ice
