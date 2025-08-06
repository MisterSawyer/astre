#pragma once

#include "async/async.hpp"
#include "process/process.hpp"

#include "ecs/component_type.hpp"
#include "ecs/registry.hpp"

namespace astre::ecs::system
{
    class SystemBase
    {
        public:
            virtual ~SystemBase() = default;

            virtual std::vector<std::type_index> getReads() const = 0;
        
            virtual std::vector<std::type_index> getWrites() const = 0;
    };

    template<class ComponentType>
    class System : public SystemBase
    {
        public:
            using component_type = ComponentType;

            static constexpr std::uint32_t MASK_BIT = ComponentTypesList::template getTypeID<component_type>();

            inline System(Registry & registry)
                : _registry(registry)
            {}

            inline System(System && other) 
            : _registry(other._registry)
            {}

            inline System & operator=(System && other) = delete;

            System(const System &) = delete;
            System & operator=(const System &) = delete;

            virtual ~System() = default;

        protected:
            Registry & getRegistry() { return _registry; }

            template<typename Tuple>
            static std::vector<std::type_index> expand()
            {
                std::vector<std::type_index> out;
                std::apply([&](auto... Ts) {
                    (out.emplace_back(std::type_index(typeid(Ts))), ...);
                }, Tuple{});
                return out;
            }

        private:
            Registry & _registry;
    };
}