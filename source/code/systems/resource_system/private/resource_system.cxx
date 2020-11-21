#include <ice/resource_system.hxx>
#include <ice/resource_index.hxx>
#include <ice/resource_meta.hxx>
#include <ice/pod/hash.hxx>
#include <ice/map.hxx>

namespace ice
{

    class SimpleResourceSystem : public ResourceSystem
    {
    public:
        SimpleResourceSystem(ice::Allocator& alloc) noexcept
            : ice::ResourceSystem{ }
            , _allocator{ alloc }
            , _index_list{ _allocator }
            , _index_map{ _allocator }
            , _known_names{ _allocator }
        { }

        ~SimpleResourceSystem() noexcept override
        { }

        void register_index(
            ice::StringID_Arg scheme,
            ice::UniquePtr<ice::ResourceIndex> index
        ) noexcept
        {
            ice::pod::multi_hash::insert(_index_map, ice::hash(scheme), index.get());
            _index_list.push_back(std::move(index));
        }

        auto locate_or_passthrough(ice::URI const& uri) noexcept -> ice::URI const&
        {
            if (ice::stringid_hash(uri.scheme) == ice::stringid_hash(ice::scheme_resource))
            {
                return ice::pod::hash::get(
                    _known_names,
                    ice::hash(uri.fragment),
                    ice::uri_invalid
                );
            }
            else
            {
                return uri;
            }
        }

        auto locate(ice::URN urn) const noexcept -> ice::URI override
        {
            return ice::pod::hash::get(_known_names, ice::hash(urn.name), ice::uri_invalid);
        }

        auto mount(ice::URI const& uri) noexcept -> ice::u32 override
        {
            if (ice::stringid_hash(uri.scheme) == ice::stringid_hash(ice::scheme_resource))
            {
                // #todo assert, invalid URI
                return 0;
            }

            ice::u64 const scheme_hash = ice::hash(uri.scheme);
            ice::ResourceIndex* mounting_index = nullptr;

            auto index_entry = ice::pod::multi_hash::find_first(_index_map, scheme_hash);
            while (index_entry != nullptr && mounting_index == nullptr)
            {
                if (index_entry->value->mount(uri))
                {
                    mounting_index = index_entry->value;
                }

                // Get the next entry
                index_entry = ice::pod::multi_hash::find_next(_index_map, index_entry);
            }

            if (mounting_index != nullptr)
            {
                ice::ResourceQuery query;
                mounting_index->query_changes(query);

                ice::u32 const object_count = static_cast<ice::u32>(query.objects.size());
                for (ice::u32 idx = 0; idx < object_count; ++idx)
                {
                    ice::Resource const* resource = query.objects[idx];
                    ice::StringID const resouce_name = ice::stringid(resource->name());

                    if (ice::pod::hash::has(_known_names, ice::hash(resouce_name)))
                    {
                        // #todo push duplicate/replace event
                    }

                    ice::pod::hash::set(
                        _known_names,
                        ice::hash(resouce_name),
                        resource->location()
                    );
                }

                return object_count;
            }
            return 0;
        }

        auto request(ice::URI const& uri) noexcept -> ice::Resource* override
        {
            ice::URI const& located_uri = locate_or_passthrough(uri);
            ice::Resource* result = nullptr;

            auto* entry = ice::pod::multi_hash::find_first(_index_map, ice::hash(located_uri.scheme));
            while (entry != nullptr && result == nullptr)
            {
                result = entry->value->request(located_uri);
                entry = ice::pod::multi_hash::find_next(_index_map, entry);
            }

            return result;
        }

        void release(ice::URI const& uri) noexcept override
        {
            ice::URI const& located_uri = locate_or_passthrough(uri);

            auto* entry = ice::pod::multi_hash::find_first(_index_map, ice::hash(located_uri.scheme));
            while (entry != nullptr)
            {
                if (entry->value->release(located_uri))
                {
                    break;
                }

                entry = ice::pod::multi_hash::find_next(_index_map, entry);
            }
        }

        auto open(ice::URI const& uri) noexcept -> ice::Sink* override
        {
            return nullptr;
        }

        void close(ice::URI const& uri) noexcept override
        {
        }

    private:
        ice::Allocator& _allocator;

        ice::Vector<ice::UniquePtr<ice::ResourceIndex>> _index_list;
        ice::pod::Hash<ice::ResourceIndex*> _index_map;

        ice::pod::Hash<ice::URI> _known_names;
    };

    auto create_resource_system(ice::Allocator& alloc) noexcept -> ice::UniquePtr<ResourceSystem>
    {
        return ice::make_unique<ice::ResourceSystem, ice::SimpleResourceSystem>(alloc, alloc);
    }

} // namespace ice
