#pragma once

#include "async/async.hpp"
#include "process/process.hpp"

#include "ecs/component_type.hpp"
#include "ecs/registry.hpp"

namespace astre::ecs::system
{
    template<class ComponentType>
    class System
    {
        public:
            using component_type = ComponentType;

            static constexpr std::uint32_t MASK_BIT = ComponentTypesList::template getTypeID<component_type>();

            inline System(Registry & registry, process::IProcess::execution_context_type & execution_context)
                : _registry(registry),
                  _async_context(execution_context)
            {}

            inline System(System && other) 
            : _registry(other._registry), _async_context(std::move(other._async_context))
            {}

            inline System & operator=(System && other) = delete;

            System(const System &) = delete;
            System & operator=(const System &) = delete;

            virtual ~System() = default;

        protected:
            Registry & getRegistry() { return _registry; }
            async::AsyncContext<process::IProcess::execution_context_type> & getAsyncContext() { return _async_context; }

        private:
            async::AsyncContext<process::IProcess::execution_context_type> _async_context;

            Registry & _registry;
    };
}