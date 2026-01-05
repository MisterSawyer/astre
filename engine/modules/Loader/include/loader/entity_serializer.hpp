#include <filesystem>
#include <functional>
#include <fstream>

#include "native/native.h"
#include <asio.hpp>
#include <absl/container/flat_hash_map.h>

#include "ecs/entity.hpp"
#include "ecs/registry.hpp"

namespace astre::loader
{
    /**
     * Serialize component, from entity in registry, into entity definition
    */
    using ComponentSerializer = std::function<asio::awaitable<void>(const ecs::Entity, const ecs::Registry &, proto::ecs::EntityDefinition &)>;

    class EntitySerializer
    {
    public:
        EntitySerializer();
        inline EntitySerializer(EntitySerializer && other) = default;
        inline EntitySerializer& operator=(EntitySerializer && other) = default;
        EntitySerializer(const EntitySerializer &) = delete;
        EntitySerializer& operator=(const EntitySerializer &) = delete;

        asio::awaitable<proto::ecs::EntityDefinition> serializeEntity(const ecs::Entity entity, const ecs::Registry & registry) const;

    private:
        void _registerComponentSerializer(const std::string& name, ComponentSerializer serializer);

        absl::flat_hash_map<std::string, ComponentSerializer> _serializers;
    };
}