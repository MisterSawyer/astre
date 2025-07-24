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
    using ComponentSerializer = std::function<void(const ecs::Entity, const ecs::Registry &, ecs::EntityDefinition &)>;

    class ComponentSerializerRegistry
    {
    public:
        ComponentSerializerRegistry();
        inline ComponentSerializerRegistry(ComponentSerializerRegistry && other) = default;
        inline ComponentSerializerRegistry& operator=(ComponentSerializerRegistry && other) = default;
        ComponentSerializerRegistry(const ComponentSerializerRegistry &) = delete;
        ComponentSerializerRegistry& operator=(const ComponentSerializerRegistry &) = delete;

        ecs::EntityDefinition serializeEntity(ecs::Entity entity, const ecs::Registry & registry) const;

    private:
        void registerSerializer(const std::string& name, ComponentSerializer serializer);

        absl::flat_hash_map<std::string, ComponentSerializer> _serializers;
    };
}