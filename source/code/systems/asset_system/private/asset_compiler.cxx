#include <asset_system/asset_compiler.hxx>

namespace asset
{

    AssetCompilationResult::AssetCompilationResult(core::allocator& alloc) noexcept
        : data{ alloc }
        , metadata{ alloc }
        , dependencies{ alloc }
    {
    }

} // namespace asset
