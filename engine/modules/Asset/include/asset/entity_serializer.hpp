#include <filesystem>
#include <functional>
#include <fstream>

#include "native/native.h"
#include <asio.hpp>
#include <absl/container/flat_hash_map.h>

#include "ecs/ecs.hpp"

namespace astre::asset
{
    /**
     * Serialize component, from entity in registry, into entity definition
    */
    using ComponentSerializer = std::function<asio::awaitable<void>(const ecs::Entity, const ecs::Registry &, ecs::EntityDefinition &)>;

    class EntitySerializer
    {
    public:
        EntitySerializer(const ecs::Registry & registry);
        inline EntitySerializer(EntitySerializer && other) = default;
        inline EntitySerializer& operator=(EntitySerializer && other) = default;
        EntitySerializer(const EntitySerializer &) = delete;
        EntitySerializer& operator=(const EntitySerializer &) = delete;

        asio::awaitable<ecs::EntityDefinition> serializeEntity(ecs::Entity entity) const;

    private:
        void registerComponentSerializer(const std::string& name, ComponentSerializer serializer);

        const ecs::Registry & _registry;
        absl::flat_hash_map<std::string, ComponentSerializer> _serializers;
    };
}