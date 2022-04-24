#include <ice/world/world_trait_archive.hxx>
#include <ice/pod/hash.hxx>
#include <ice/assert.hxx>

namespace ice
{

    class SimpleWorldTraitArchive : public ice::WorldTraitArchive
    {
    public:
        SimpleWorldTraitArchive(ice::Allocator& alloc) noexcept;

        void register_trait(
            ice::StringID_Arg name,
            ice::WorldTraitDescription description
        ) noexcept override;

        auto find_trait(
            ice::StringID_Arg name
        ) const noexcept -> ice::WorldTraitDescription const* override;

    private:
        ice::Allocator& _allocator;
        ice::pod::Hash<ice::WorldTraitDescription> _trait_descriptions;
    };

    SimpleWorldTraitArchive::SimpleWorldTraitArchive(ice::Allocator& alloc) noexcept
        : _allocator{ alloc }
        , _trait_descriptions{ alloc }
    {
    }

    void SimpleWorldTraitArchive::register_trait(
        ice::StringID_Arg name,
        ice::WorldTraitDescription description
    ) noexcept
    {
        ice::u64 const name_hash = ice::hash(name);

        ICE_ASSERT(
            ice::pod::hash::has(_trait_descriptions, name_hash) == false,
            "Trait with this name already exists!"
        );

        ice::pod::hash::set(
            _trait_descriptions,
            name_hash,
            description
        );
    }

    auto SimpleWorldTraitArchive::find_trait(
        ice::StringID_Arg name
    ) const noexcept -> ice::WorldTraitDescription const*
    {
        ice::u64 const name_hash = ice::hash(name);

        static ice::WorldTraitDescription invalid{ .factory = nullptr };
        ice::WorldTraitDescription const& description = ice::pod::hash::get(_trait_descriptions, name_hash, invalid);
        return description.factory == nullptr ? nullptr : &description;
    }

    auto create_world_trait_archive(
        ice::Allocator& alloc
    ) noexcept -> ice::UniquePtr<ice::WorldTraitArchive>
    {
        return ice::make_unique<ice::WorldTraitArchive, ice::SimpleWorldTraitArchive>(alloc, alloc);
    }

} // namespace ice
