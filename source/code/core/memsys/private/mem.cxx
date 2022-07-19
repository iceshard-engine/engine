#include <ice/mem.hxx>
#include <assert.h>

namespace ice
{
    auto alloc(ice::usize request) noexcept -> ice::alloc_result
    {
#if ISP_WINDOWS || ISP_UNIX
        return ice::alloc_result{
            .result = malloc(request.value),
            .size = request,
            .alignment = ice::build::is_x64 ? ice::ualign::b_16 : ice::ualign::b_8,
        };
#else
        assert(false);
        return nullptr;
#endif
    }

    void release(ice::alloc_result alloc_result) noexcept
    {
#if ISP_WINDOWS || ISP_UNIX
        assert(alloc_result.alignment == (ice::build::is_x64 ? ice::ualign::b_16 : ice::ualign::b_8));
        free(alloc_result.result);
#else
        assert(false);
#endif
    }

    auto alloc_aligned(ice::alloc_request request) noexcept -> ice::alloc_result
    {
#if ISP_WINDOWS
        return ice::alloc_result{
            .result = _aligned_malloc(request.size.value, static_cast<ice::u32>(request.alignment)),
            .size = request.size,
            .alignment = request.alignment,
        };
#elif ISP_UNIX
        return ice::alloc_result{
            .result = aligned_alloc(static_cast<ice::u32>(request.alignment), request.size.value),
            .size = request.size,
            .alignment = request.alignment,
        };
#else
        assert(false);
        return nullptr;
#endif
    }

    void release_aligned(ice::alloc_result alloc_result) noexcept
    {
#if ISP_WINDOWS
        _aligned_free(alloc_result.result);
#elif ISP_UNIX
        free(alloc_result.result);
#else
        assert(false);
        return nullptr;
#endif
    }

    // Additional overloads
    void release(void* pointer) noexcept
    {
        release(
            ice::alloc_result{ .result = pointer, .alignment = ice::ualign::b_default }
        );
    }

    void release_aligned(void* pointer, ice::ualign alignment) noexcept
    {
        release_aligned(
            ice::alloc_result{ .result = pointer, .alignment = alignment }
        );
    }

} // namespace ice
