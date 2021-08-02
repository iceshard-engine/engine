#pragma once
#include <ice/base.hxx>
#include <ice/hash.hxx>
#include <string_view>

namespace ice
{

    struct Shard
    {
        ice::u32 name;
        ice::u32 payload_id;
        ice::u64 payload;
    };

    constexpr auto shard_create(std::string_view sv) noexcept  -> ice::Shard;

    template<typename T>
    constexpr auto shard_create(std::string_view sv, T value) noexcept  -> ice::Shard;

    template<typename T>
    constexpr auto shard_create(ice::Shard sv, T value) noexcept  -> ice::Shard;

    template<typename T>
    constexpr bool shard_inspect(ice::Shard shard, T& value) noexcept;

    template<typename T>
    inline auto shard_shatter(ice::Shard shard) noexcept -> T;

    template<ice::Shard... TestShards>
    constexpr bool any_of(ice::Shard const& shard) noexcept
    {
        return ((shard == TestShards) || ...);
    }

    namespace detail
    {

        template<typename T>
        constexpr ice::u32 Constant_ShardPayloadID = 0;

        template<>
        constexpr ice::u32 Constant_ShardPayloadID<ice::i32> = ice::hash32("ice::i32");

        template<>
        constexpr ice::u32 Constant_ShardPayloadID<ice::u32> = ice::hash32("ice::u32");

        template<>
        constexpr ice::u32 Constant_ShardPayloadID<ice::i64> = ice::hash32("ice::i64");

        template<>
        constexpr ice::u32 Constant_ShardPayloadID<ice::u64> = ice::hash32("ice::u64");

        template<>
        constexpr ice::u32 Constant_ShardPayloadID<ice::math::vec2i> = ice::hash32("ice::math::vec2i");

        template<>
        constexpr ice::u32 Constant_ShardPayloadID<ice::math::vec2u> = ice::hash32("ice::math::vec2u");

    } // namespace detail

    constexpr auto shard_create(std::string_view sv) noexcept  -> ice::Shard
    {
        return ice::Shard{
            .name = ice::hash32(sv),
            .payload_id = 0,
            .payload = 0,
        };
    }

    template<typename T>
    constexpr auto shard_create(std::string_view sv, T payload) noexcept -> ice::Shard
    {
        static_assert(sizeof(T) <= sizeof(ice::Shard::payload), "The given payload is bigger than a shard can have attached.");
        static_assert(ice::detail::Constant_ShardPayloadID<T> != 0, "The given type cannot be used to attach a shard payload.");
        if constexpr (std::is_pointer_v<T> == false)
        {
            return {
                .name = ice::hash32(sv),
                .payload_id = ice::detail::Constant_ShardPayloadID<T>,
                .payload = static_cast<ice::u64>(payload)
            };
        }
        else
        {
            return {
                .name = ice::hash32(sv),
                .payload_id = ice::detail::Constant_ShardPayloadID<T>,
                .payload = *reinterpret_cast<ice::u64 const*>(ice::addressof(payload))
            };
        }
    }

    template<typename T>
    constexpr auto shard_create(ice::Shard shard, T payload) noexcept  -> ice::Shard
    {
        static_assert(sizeof(T) <= sizeof(ice::Shard::payload), "The given payload is bigger than a shard can have attached.");
        static_assert(ice::detail::Constant_ShardPayloadID<T> != 0, "The given type cannot be used to attach a shard payload.");
        if constexpr (std::is_pointer_v<T> == false && std::is_class_v<T> == false)
        {
            return {
                .name = shard.name,
                .payload_id = ice::detail::Constant_ShardPayloadID<T>,
                .payload = static_cast<ice::u64>(payload)
            };
        }
        else
        {
            return {
                .name = shard.name,
                .payload_id = ice::detail::Constant_ShardPayloadID<T>,
                .payload = *reinterpret_cast<ice::u64 const*>(ice::addressof(payload))
            };
        }
    }

    template<typename T>
    constexpr bool shard_inspect(ice::Shard shard, T& value) noexcept
    {
        static_assert(ice::detail::Constant_ShardPayloadID<T> != 0, "The given type cannot be used to inspect a shard object.");
        if (ice::detail::Constant_ShardPayloadID<T> == shard.payload_id)
        {
            value = static_cast<T>(shard.payload);
            return true;
        }
        return false;
    }

    template<typename T>
    inline bool shard_inspect(ice::Shard shard, T*& value) noexcept
    {
        static_assert(ice::detail::Constant_ShardPayloadID<T*> != 0, "The given type cannot be used to inspect a shard object.");
        if (ice::detail::Constant_ShardPayloadID<T*> == shard.payload_id)
        {
            value = reinterpret_cast<T*>(static_cast<uptr>(shard.payload));
            return true;
        }
        return false;
    }

    template<typename T>
    inline auto shard_shatter(ice::Shard shard) noexcept -> T
    {
        static_assert(ice::detail::Constant_ShardPayloadID<T> != 0, "The given type cannot be used to shatter a shard object.");
        return *reinterpret_cast<T const*>(ice::addressof(shard.payload));
    }

    constexpr auto operator""_shard(const char* str, size_t size) noexcept
    {
        return ice::shard_create({ str, size });
    }

    constexpr auto operator""_shard_name(const char* str, size_t size) noexcept
    {
        return ice::shard_create({ str, size }).name;
    }

    template<typename T>
    constexpr auto operator|(ice::Shard shard, T payload) noexcept -> ice::Shard
    {
        return ice::shard_create(shard, payload);
    }

    template<typename T>
    constexpr auto operator|(ice::Shard shard, T* payload) noexcept -> ice::Shard
    {
        return ice::shard_create(shard, payload);
    }

    constexpr auto operator>>(ice::Shard left, ice::Shard right) noexcept -> ice::Shard
    {
        return {
            .name = right.name,
            .payload_id = left.payload_id,
            .payload = left.payload
        };
    }

    constexpr auto operator==(ice::Shard left, ice::Shard right) noexcept -> bool
    {
        if (left.name == right.name)
        {
            return left.payload_id == 0 || right.payload_id == 0 || right.payload_id == left.payload_id;
        }
        return false;
    }

    constexpr auto operator!=(ice::Shard left, ice::Shard right) noexcept -> bool
    {
        return !(left == right);
    }

    namespace _validate
    {

        static constexpr ice::Shard shard_without_payload = "test/shard"_shard;
        static constexpr ice::Shard shard_with_payload_u32 = "test/shard"_shard | ice::u32{ 42 };
        static constexpr ice::Shard shard_with_payload_i32 = "test/shard"_shard | ice::i32{ 42 };

        static constexpr ice::Shard shard2_without_payload = "test/shard2"_shard;
        static constexpr ice::Shard shard2_with_payload_u32 = "test/shard2"_shard | ice::u32{ 42 };
        static constexpr ice::Shard shard2_with_payload_i32 = "test/shard2"_shard | ice::i32{ 42 };

        static_assert(shard_without_payload == shard_without_payload, "Assert: Shards are equal to themselfs.");
        static_assert(shard_with_payload_u32 == shard_with_payload_u32, "Assert: Shards are equal to themselfs.");

        static_assert(shard_without_payload == shard_with_payload_u32, "Assert: Shards without payload are equal to similar Shards with payload.");
        static_assert(shard_without_payload == shard_with_payload_i32, "Assert: Shards without payload are equal to similar Shards with payload.");
        static_assert(shard_with_payload_u32 == shard_without_payload, "Assert: Shards without payload are equal to similar Shards with payload.");
        static_assert(shard_with_payload_i32 == shard_without_payload, "Assert: Shards without payload are equal to similar Shards with payload.");

        static_assert(shard_with_payload_i32 != shard_with_payload_u32, "Assert: Shards with different payloads are not equal.");
        static_assert(shard_with_payload_u32 != shard_with_payload_i32, "Assert: Shards with different payloads are not equal.");

        static_assert(shard_without_payload != shard2_without_payload, "Assert: Different Shards are not equal.");

        static_assert(shard2_with_payload_u32 != shard_with_payload_u32, "Assert: Different Shards with same payloads are not equal.");
        static_assert(shard2_with_payload_i32 != shard_with_payload_i32, "Assert: Different Shards with same payloads are not equal.");

        static_assert(shard2_with_payload_u32 != shard_with_payload_i32, "Assert: Different Shards with different payloads are not equal.");
        static_assert(shard2_with_payload_i32 != shard_with_payload_u32, "Assert: Different Shards with different payloads are not equal.");

    } // namespace detail

} // namespace ice

//
//namespace ice
//{
//
//    namespace detail_2
//    {
//
//        enum class Shard : ice::u64 { Invalid = 0x0 };
//
//        template<ice::u32 Size = 1>
//        struct ShardCluster
//        {
//            Shard shard;
//            ice::u64 payload[Size];
//        };
//
//        struct ShardInfo
//        {
//            ice::u32 name;
//            ice::u32 command : 30;
//            ice::u32 magic : 2;
//        };
//
//        constexpr auto create_shard(std::string_view sv) noexcept
//        {
//            ice::u64 const cmd_pos = sv.find_first_of('|');
//            ice::u64 const name_hash = ice::hash32(sv.substr(0, cmd_pos));
//            ice::u64 cmd_hash = 0;
//
//            if (cmd_pos != std::string_view::npos)
//            {
//                cmd_hash = ice::hash32(sv.substr(cmd_pos));
//            }
//
//            return Shard{ (name_hash << 32) | (cmd_hash & 0xffff'fffc) | 0 };
//        }
//
//        template<typename T>
//        constexpr auto create_shard_with_payload(std::string_view sv) noexcept
//        {
//            return Shard{ static_cast<ice::u64>(create_shard(sv)) | ((sizeof(T) - 1) / 8) + 1 };
//        }
//
//        constexpr auto operator""_sh(const char* str, size_t size) noexcept
//        {
//            return create_shard({ str, size });
//        }
//
//        constexpr auto shard_info(Shard shard) noexcept -> ShardInfo
//        {
//            ice::u64 const shard_value = static_cast<ice::u64>(shard);
//            ice::u32 const name_hash = static_cast<ice::u32>(shard_value >> 32);
//            ice::u32 const cmd_hash = static_cast<ice::u32>(shard_value) & 0xffff'fffc;
//            ice::u32 const magic = static_cast<ice::u32>(shard_value) & ~0xffff'fffc;
//            return ShardInfo{ name_hash, cmd_hash, magic };
//        }
//
//        constexpr auto shard_magic(Shard shard) noexcept -> ice::u32
//        {
//            return static_cast<ice::u64>(shard) & 3;
//        }
//
//        template<Shard, typename T>
//        auto inspect_cluster(void* payload) noexcept -> T = delete;
//
//        template<Shard shard, typename T>
//        auto create_cluster(T const&) noexcept -> ShardCluster<shard_magic(shard)> = delete;
//
//        //template<typename T>
//        //static constexpr ice::Shard Shard = Shard::Invalid;
//
//
//        //template<>
//        //using PayloadType<"entity|create"_sh> = ice::u32;
//
//        auto constexpr Shard_EntityCreate = create_shard_with_payload<ice::u64>("entity|created");
//        auto constexpr Shard_EntityDestroy = create_shard("entity|destroyed");
//
//        template<>
//        inline auto inspect_cluster<Shard_EntityCreate>(void* payload) noexcept -> ice::u64
//        {
//            return *reinterpret_cast<ice::u32*>(payload);
//        }
//
//        template<>
//        inline auto create_cluster<Shard_EntityCreate>(ice::u64 const& v) noexcept -> ShardCluster<1>
//        {
//            return { Shard_EntityCreate, v };
//        }
//
//        auto constexpr ai = shard_info(Shard_EntityCreate);
//        auto constexpr bi = shard_info(Shard_EntityDestroy);
//
//    } // namespace detail_2
//
//    namespace detail
//    {
//
//        template<ice::u32 Size>
//        struct ClusterBase;
//
//    }
//
//    //template<typename T, ice::u32 Size>
//    //constexpr auto create_cluster(T value) noexcept -> ice::detail::ClusterBase<Size> = delete;
//
//    template<typename T>
//    constexpr auto create_cluster(T value) noexcept -> ice::detail::ClusterBase<1> = delete;
//
//    template<typename T, ice::u32 Size>
//    constexpr auto create_cluster(T const& value) noexcept -> ice::detail::ClusterBase<Size> = delete;
//
//    //enum class ShardType : ice::u32
//    //{
//    //    Shard,
//    //    Cluster,
//    //    Blizzard,
//    //};
//
//    namespace detail
//    {
//
//        enum class Shard_HashType : ice::u32
//        {
//            Invalid = 0x0
//        };
//
//        struct ShardBase
//        {
//            union
//            {
//                struct
//                {
//                    //ice::ShardType type : 2;
//                    ice::detail::Shard_HashType name : 32;
//                    ice::u32 cluster_shards : 2;
//                    ice::u32 payload : 30;
//                };
//                ice::u64 cluster_payload;
//            };
//        };
//
//        template<ice::u32 Size>
//        struct ClusterBase
//        {
//            static_assert(Size <= 4, "A cluster cannot hold more than 4 shards!");
//            ice::detail::ShardBase shards[Size]{ };
//        };
//
//        template<ice::u32 Size>
//        ClusterBase(ShardBase, ClusterBase<Size>) -> ClusterBase<Size + 1>;
//
//        constexpr auto shard_base(ice::u32 name_hash, ice::u32 payload) noexcept
//        {
//            ShardBase shard{ };
//            //shard.type = ice::ShardType::Shard;
//            shard.name = Shard_HashType{ name_hash };
//            shard.cluster_shards = 0;
//            shard.payload = payload;
//            return shard;
//        }
//
//        constexpr auto shard_base(ice::u64 payload) noexcept
//        {
//            return ShardBase{
//                .cluster_payload = payload
//            };
//        }
//
//        static_assert(
//            sizeof(ShardBase) == 8,
//            "Shared base type is not 8 bytes big!"
//        );
//
//        constexpr auto operator==(ShardBase left, ShardBase right) noexcept
//        {
//            return left.name == right.name;
//        }
//
//        constexpr auto operator!=(ShardBase left, ShardBase right) noexcept
//        {
//            return !(left == right);
//        }
//
//        //constexpr auto operator==(ShardBase left, ShardType right) noexcept
//        //{
//        //    return left.type == right;
//        //}
//
//        //constexpr auto operator!=(ShardBase left, ShardType right) noexcept
//        //{
//        //    return !(left == right);
//        //}
//
//
//        //constexpr auto cluster(ice::detail::ShardBase shard, void* ptr) noexcept -> ice::detail::ClusterBase<2>
//        //{
//        //    shard.type = ice::ShardType::Cluster;
//        //    shard.cluster_shards = 1;
//        //    return { shard, detail::shard_base(static_cast<ice::u64>(reinterpret_cast<ice::uptr>(ptr))) };
//        //}
//
//        //constexpr auto operator|(ice::detail::ShardBase shard, void* ptr) noexcept
//        //{
//        //    return ice::detail::cluster(shard, ptr);
//        //}
//
//        template<typename T, ice::u32 Size>
//        concept IsShardValue = requires(T t) {
//            { create_cluster<T>(t) } -> std::convertible_to<ice::detail::ClusterBase<1>>;
//        } || requires(T t) {
//            { create_cluster<T, Size>(t) } -> std::convertible_to<ice::detail::ClusterBase<Size>>;
//        };
//
//        template<typename T> requires ice::detail::IsShardValue<T, 1>
//        constexpr auto operator|(ice::detail::ShardBase shard, T value) noexcept
//        {
//            ClusterBase<1> cluster = create_cluster<T, 1>(value);
//
//            ClusterBase<2> result{ };
//            result.shards[0] = shard;
//            //result.shards[0].type = ShardType::Cluster;
//            result.shards[0].cluster_shards = 1;
//            for (ice::u32 idx = 1; idx < 2; ++idx)
//            {
//                result.shards[idx] = cluster.shards[idx - 1];
//            }
//
//            return result;
//        }
//
//        template<typename T> requires ice::detail::IsShardValue<T, 2>
//        constexpr auto operator|(ice::detail::ShardBase shard, T const& value) noexcept
//        {
//            ClusterBase<2> cluster = create_cluster<T, 2>(value);
//
//            ClusterBase<3> result{ };
//            result.shards[0] = shard;
//            //result.shards[0].type = ShardType::Cluster;
//            result.shards[0].cluster_shards = 2;
//            for (ice::u32 idx = 1; idx < 3; ++idx)
//            {
//                result.shards[idx] = cluster.shards[idx - 1];
//            }
//
//            return result;
//        }
//
//
//        template<typename T> requires ice::detail::IsShardValue<T, 3>
//        constexpr auto operator|(ice::detail::ShardBase shard, T const& value) noexcept
//        {
//            ClusterBase<3> cluster = create_cluster<T, 3>(value);
//
//            ClusterBase<4> result{ };
//            result.shards[0] = shard;
//            //result.shards[0].type = ShardType::Cluster;
//            result.shards[0].cluster_shards = 3;
//            for (ice::u32 idx = 1; idx < 4; ++idx)
//            {
//                result.shards[idx] = cluster.shards[idx - 1];
//            }
//
//            return result;
//        }
//
//    } // namespace detail
//
//    using Shard = ice::detail::ShardBase;
//
//    template<ice::u32 Size = 1>
//    using Cluster = ice::detail::ClusterBase<Size>;
//
//
//    constexpr auto create_shard(std::string_view str) noexcept
//    {
//        auto const payload_pos = str.find_first_of('|');
//        if (payload_pos != std::string_view::npos)
//        {
//            return detail::shard_base(
//                ice::hash32(str.substr(0, payload_pos)),
//                ice::hash32(str.substr(payload_pos + 1))
//            );
//        }
//        else
//        {
//            return detail::shard_base(ice::hash32(str), 0);
//        }
//    }
//
//    //template<typename T>
//    //constexpr auto create_cluster(ice::Shard shard, T value) noexcept -> Cluster<2> = delete;
//
//
//    constexpr auto operator""_shard(const char* str, std::size_t size) noexcept -> ice::Shard
//    {
//        return ice::create_shard({ str, size });
//    }
//
//    struct HolyFuck
//    {
//        ice::u64 a;
//        ice::u64 b;
//    };
//
//    template<>
//    constexpr auto create_cluster<ice::i32>(ice::i32 const& value) noexcept -> ice::detail::ClusterBase<1>
//    {
//        return Cluster<1>{ Shard{ .cluster_payload = static_cast<ice::u32>(value) } };
//    }
//
//    template<>
//    constexpr auto create_cluster<ice::HolyFuck, 2>(ice::HolyFuck const& v) noexcept -> ice::detail::ClusterBase<2>
//    {
//        return Cluster<2>{
//            Shard{ .cluster_payload = static_cast<ice::u32>(v.a) },
//            Shard{ .cluster_payload = static_cast<ice::u32>(v.b) }
//        };
//    }
//
//    constexpr bool v = ice::detail::IsShardValue<ice::i32, 1>;
//
//    static constexpr auto cc = "test|a"_shard | HolyFuck{ .a = 1, .b = 2 };
//    static constexpr auto cc2 = "test|b"_shard | 22;
//
//} // namespace ice
