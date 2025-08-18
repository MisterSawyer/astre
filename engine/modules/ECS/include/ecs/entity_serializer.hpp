#include <filesystem>
#include <functional>
#include <fstream>

#include "native/native.h"
#include <asio.hpp>
#include <absl/container/flat_hash_map.h>

#include "ecs/entity.hpp"
#include "ecs/registry.hpp"

namespace astre::ecs
{
    /**
     * Serialize component, from entity in registry, into entity definition
    */
    using ComponentSerializer = std::function<asio::awaitable<void>(const Entity, const Registry &, EntityDefinition &)>;

    class EntitySerializer
    {
    public:
        EntitySerializer();
        inline EntitySerializer(EntitySerializer && other) = default;
        inline EntitySerializer& operator=(EntitySerializer && other) = default;
        EntitySerializer(const EntitySerializer &) = delete;
        EntitySerializer& operator=(const EntitySerializer &) = delete;

        asio::awaitable<EntityDefinition> serializeEntity(const Entity entity, const Registry & registry) const;

    private:
        void _registerComponentSerializer(const std::string& name, ComponentSerializer serializer);

        absl::flat_hash_map<std::string, ComponentSerializer> _serializers;
    };
}