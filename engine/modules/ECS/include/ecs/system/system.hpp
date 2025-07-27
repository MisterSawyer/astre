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

            static constexpr const std::uint32_t MASK_BIT = ComponentTypesList::template getTypeID<component_type>();

            System(Registry & registry, process::IProcess::execution_context_type & execution_context)
                : _registry(registry),
                  _async_context(execution_context)
            {}
            
            virtual ~System() = default;

        protected:
            Registry & getRegistry() { return _registry; }
            async::AsyncContext<process::IProcess::execution_context_type> & getAsyncContext() { return _async_context; }

        private:
            async::AsyncContext<process::IProcess::execution_context_type> _async_context;

            Registry & _registry;
    };
}