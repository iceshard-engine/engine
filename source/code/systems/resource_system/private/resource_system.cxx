#include <ice/resource_system.hxx>
#include <ice/resource_index.hxx>
#include <ice/resource_meta.hxx>
#include <ice/resource_query.hxx>
#include <ice/pod/hash.hxx>
#include <ice/map.hxx>

namespace ice
{

    class SimpleResourceSystem : public ResourceSystem
    {
    public:
        SimpleResourceSystem(ice::Allocator& alloc) noexcept;
        ~SimpleResourceSystem() noexcept override = default;

        void register_index(
            ice::Span<ice::StringID> schemes,
            ice::UniquePtr<ice::ResourceIndex> index
        ) noexcept override;

        bool query_changes(ice::ResourceQuery& query) noexcept override;

        auto locate_or_passthrough(ice::URI const& uri) noexcept -> ice::URI const&;

        auto locate(ice::URN urn) const noexcept -> ice::URI override;

        auto mount(ice::URI const& uri) noexcept -> ice::u32 override;

        auto request(ice::URI const& uri) noexcept -> ice::Resource* override;

        void release(ice::URI const& uri) noexcept override;

        auto open(ice::URI const& uri) noexcept -> ice::Sink* override;

        void close(ice::URI const& uri) noexcept override;

    private:
        ice::Allocator& _allocator;

        ice::Vector<ice::UniquePtr<ice::ResourceIndex>> _index_list;
        ice::pod::Hash<ice::ResourceIndex*> _index_map;

        ice::pod::Hash<ice::Resource const*> _known_resources;

        ice::pod::Array<ice::ResourceEvent> _events;
        ice::pod::Array<ice::Resource*> _event_objects;
    };

    SimpleResourceSystem::SimpleResourceSystem(ice::Allocator& alloc) noexcept
        : ice::ResourceSystem{ }
        , _allocator{ alloc }
        , _index_list{ _allocator }
        , _index_map{ _allocator }
        , _known_resources{ _allocator }
        , _events{ _allocator }
        , _event_objects{ _allocator }
    { }

    void SimpleResourceSystem::register_index(
        ice::Span<ice::StringID> schemes,
        ice::UniquePtr<ice::ResourceIndex> index
    ) noexcept
    {
        for (ice::StringID const& scheme : schemes)
        {
            ice::pod::multi_hash::insert(_index_map, ice::hash(scheme), index.get());
        }
        _index_list.push_back(std::move(index));
    }

    bool SimpleResourceSystem::query_changes(ice::ResourceQuery& query) noexcept
    {
        if (ice::pod::array::any(_events))
        {
            query.events = ice::move(_events);
            query.objects = ice::move(_event_objects);
            return true;
        }
        return false;
    }

    auto SimpleResourceSystem::locate_or_passthrough(ice::URI const& uri) noexcept -> ice::URI const&
    {
        if (ice::stringid_hash(uri.scheme) == ice::stringid_hash(ice::scheme_resource))
        {
            ice::Resource const* resource;
            if (uri.fragment == ice::stringid_invalid)
            {
                resource = ice::pod::hash::get(
                    _known_resources,
                    ice::hash(ice::stringid(uri.path)),
                    nullptr
                );
            }
            else
            {
                resource = ice::pod::hash::get(
                    _known_resources,
                    ice::hash(uri.fragment),
                    nullptr
                );
            }
            return resource == nullptr ? ice::uri_invalid : resource->location();
        }
        else
        {
            return uri;
        }
    }

    auto SimpleResourceSystem::locate(ice::URN urn) const noexcept -> ice::URI
    {
        ice::Resource const* resource = ice::pod::hash::get(
            _known_resources,
            ice::hash(urn.name),
            nullptr
        );
        return resource == nullptr ? ice::uri_invalid : resource->location();
    }

    auto SimpleResourceSystem::mount(ice::URI const& uri) noexcept -> ice::u32
    {
        if (ice::stringid_hash(uri.scheme) == ice::stringid_hash(ice::scheme_resource))
        {
            // #todo error reporting
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

            ice::u32 const object_count = ice::pod::array::size(query.objects);
            for (ice::u32 idx = 0; idx < object_count; ++idx)
            {
                if (query.events[idx] == ice::ResourceEvent::MountError)
                {
                    // #todo error reporting
                    continue;
                }

                ice::Resource const* const resource = query.objects[idx];
                ice::StringID const resouce_name = ice::stringid(resource->name());

                ice::Resource const* const known_resource = ice::pod::hash::get(
                    _known_resources,
                    ice::hash(resouce_name),
                    nullptr
                );

                if (known_resource == resource)
                {
                    ice::pod::array::push_back(_events, ice::ResourceEvent::Updated);
                    ice::pod::array::push_back(_event_objects, known_resource);
                }
                else
                {
                    if (known_resource != nullptr)
                    {
                        ice::pod::array::push_back(_events, ice::ResourceEvent::Replaced);
                        ice::pod::array::push_back(_event_objects, known_resource);
                    }

                    ice::pod::array::push_back(_events, ice::ResourceEvent::Added);
                    ice::pod::array::push_back(_event_objects, resource);
                    ice::pod::hash::set(
                        _known_resources,
                        ice::hash(resouce_name),
                        resource
                    );
                }
            }

            return object_count;
        }
        else
        {
            // #todo error reporting
        }

        return 0;
    }

    auto SimpleResourceSystem::request(ice::URI const& uri) noexcept -> ice::Resource*
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

    void SimpleResourceSystem::release(ice::URI const& uri) noexcept
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

    auto SimpleResourceSystem::open(ice::URI const& uri) noexcept -> ice::Sink*
    {
        return nullptr;
    }

    void SimpleResourceSystem::close(ice::URI const& uri) noexcept
    {
    }

    auto create_resource_system(ice::Allocator& alloc) noexcept -> ice::UniquePtr<ResourceSystem>
    {
        return ice::make_unique<ice::ResourceSystem, ice::SimpleResourceSystem>(alloc, alloc);
    }

} // namespace ice
