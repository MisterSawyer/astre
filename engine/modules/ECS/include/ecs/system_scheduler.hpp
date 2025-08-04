#pragma once

#include <absl/container/flat_hash_map.h>
#include <absl/container/flat_hash_set.h>
#include <vector>

#include "ecs/system/system.hpp"

namespace astre::ecs
{
    class SystemScheduler {
    public:
        void addSystem(system::SystemBase * system)
        {
            _systems.push_back(system);
        }

        // Topologically sort into layers
        std::vector<std::vector<system::SystemBase*>> schedule()
        {
            absl::flat_hash_map<system::SystemBase *, absl::flat_hash_set<system::SystemBase*>> deps;
            absl::flat_hash_map<system::SystemBase *, unsigned int> in_degree;

            // Build dependency graph
            for (const auto & A : _systems) {
                const auto A_writes = A->getWrites();
                for (const auto & B : _systems) {
                    if (A == B) continue;
                    auto B_reads = B->getReads();
                    auto B_writes = B->getWrites();

                    if (conflicts(A_writes, B_reads) || conflicts(A_writes, B_writes)) {
                        deps[A].insert(B);
                        in_degree[B]++;
                    }
                }
            }

            // Kahn's algorithm with layers
            std::queue<system::SystemBase*> q;
            for (const auto& s : _systems) {
                if (in_degree[s] == 0) q.push(s);
            }

            std::vector<std::vector<system::SystemBase *>> layers;
            while (!q.empty()) {
                std::vector<system::SystemBase*> layer;
                std::size_t n = q.size();
                for (std::size_t i = 0; i < n; ++i)
                {
                    auto s = q.front(); q.pop();
                    layer.push_back(s);
                    for (const auto & dep : deps[s]) {
                        if (--in_degree[dep] == 0)
                        {
                            q.push(dep);
                        }
                    }
                }
                layers.push_back(std::move(layer));
            }

            return layers;
        }

    private:
        std::vector<system::SystemBase*> _systems;

        static bool conflicts(const std::vector<std::type_index>& A, const std::vector<std::type_index>& B) {
            for (const auto & t : A)
            {
                if (std::find(B.begin(), B.end(), t) != B.end()) return true;
            }
            return false;
        }
    };
}