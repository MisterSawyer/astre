#include "render/vertex.hpp"

namespace astre::render
{
    namespace detail
    {
        struct VertexHasher {
            /**
             * @brief Hashes a pair of uint32_t's to a uint64_t.
             * 
             * The hash is computed by shifting the first element of the pair
             * 32 bits to the left and then performing a bitwise OR operation
             * with the second element of the pair. The resulting uint64_t is
             * then passed to std::hash<uint64_t> to produce the final hash
             * value.
             * 
             * @param p The pair of uint32_t's to hash.
             * 
             * @return The hash value for the given pair.
             */
            size_t operator()(const std::pair<uint32_t, uint32_t>& p) const noexcept {
                return std::hash<uint64_t>()((uint64_t)p.first << 32 | p.second);
            }
        };
    }

    const Mesh & getNormalizedDeviceCoordinatesQuadPrefab()
    {
        static const Mesh quad{
            // Indices (two triangles)
            {
                0, 1, 2,
                2, 3, 0
            },
            // Vertices: position, normal, texcoord
            {
                {{-1.0f, -1.0f, 0.0f}, {0, 0, 1}, {0.0f, 0.0f}}, // bottom left
                {{ 1.0f, -1.0f, 0.0f}, {0, 0, 1}, {1.0f, 0.0f}}, // bottom right
                {{ 1.0f,  1.0f, 0.0f}, {0, 0, 1}, {1.0f, 1.0f}}, // top right
                {{-1.0f,  1.0f, 0.0f}, {0, 0, 1}, {0.0f, 1.0f}}, // top left
            }
        };

        return quad;
    }

    const Mesh & getQuadPrefab()
    {
        static const Mesh quad{
            // Indices (two triangles)
            {
                0, 1, 2,
                2, 3, 0
            },
            // Vertices: position, normal, texcoord
            {
                {{-0.5f, -0.5f, 0.0f}, {0, 0, 1}, {0.0f, 0.0f}}, // bottom left
                {{ 0.5f, -0.5f, 0.0f}, {0, 0, 1}, {1.0f, 0.0f}}, // bottom right
                {{ 0.5f,  0.5f, 0.0f}, {0, 0, 1}, {1.0f, 1.0f}}, // top right
                {{-0.5f,  0.5f, 0.0f}, {0, 0, 1}, {0.0f, 1.0f}}, // top left
            }
        };

        return quad;   
    }

    const Mesh & getCubePrefab()
    {
        static const Mesh cube{
            {
                // Front (+Z)
                0, 1, 2,  3, 4, 5,
                // Back (-Z)
                6, 7, 8,  9,10,11,
                // Left (-X)
               12,13,14, 15,16,17,
                // Right (+X)
               18,19,20, 21,22,23,
                // Top (+Y)
               24,25,26, 27,28,29,
                // Bottom (-Y)
               30,31,32, 33,34,35
            },
            {
                // Front (+Z)
                {{-0.5f, -0.5f,  0.5f}, {0, 0, 1}, {0.0f, 0.0f}}, // 0
                {{ 0.5f, -0.5f,  0.5f}, {0, 0, 1}, {1.0f, 0.0f}}, // 1
                {{ 0.5f,  0.5f,  0.5f}, {0, 0, 1}, {0.5f, 1.0f}}, // 2
                {{-0.5f, -0.5f,  0.5f}, {0, 0, 1}, {0.0f, 0.0f}}, // 3
                {{ 0.5f,  0.5f,  0.5f}, {0, 0, 1}, {1.0f, 0.0f}}, // 4
                {{-0.5f,  0.5f,  0.5f}, {0, 0, 1}, {0.5f, 1.0f}}, // 5

                // Back (-Z)
                {{ 0.5f, -0.5f, -0.5f}, {0, 0, -1}, {0.0f, 0.0f}}, // 6
                {{-0.5f, -0.5f, -0.5f}, {0, 0, -1}, {1.0f, 0.0f}}, // 7
                {{-0.5f,  0.5f, -0.5f}, {0, 0, -1}, {0.5f, 1.0f}}, // 8
                {{ 0.5f, -0.5f, -0.5f}, {0, 0, -1}, {0.0f, 0.0f}}, // 9
                {{-0.5f,  0.5f, -0.5f}, {0, 0, -1}, {1.0f, 0.0f}}, //10
                {{ 0.5f,  0.5f, -0.5f}, {0, 0, -1}, {0.5f, 1.0f}}, //11

                // Left (-X)
                {{-0.5f, -0.5f, -0.5f}, {-1, 0, 0}, {0.0f, 0.0f}}, //12
                {{-0.5f, -0.5f,  0.5f}, {-1, 0, 0}, {1.0f, 0.0f}}, //13
                {{-0.5f,  0.5f,  0.5f}, {-1, 0, 0}, {0.5f, 1.0f}}, //14
                {{-0.5f, -0.5f, -0.5f}, {-1, 0, 0}, {0.0f, 0.0f}}, //15
                {{-0.5f,  0.5f,  0.5f}, {-1, 0, 0}, {1.0f, 0.0f}}, //16
                {{-0.5f,  0.5f, -0.5f}, {-1, 0, 0}, {0.5f, 1.0f}}, //17

                // Right (+X)
                {{ 0.5f, -0.5f,  0.5f}, {1, 0, 0}, {0.0f, 0.0f}},  //18
                {{ 0.5f, -0.5f, -0.5f}, {1, 0, 0}, {1.0f, 0.0f}},  //19
                {{ 0.5f,  0.5f, -0.5f}, {1, 0, 0}, {0.5f, 1.0f}},  //20
                {{ 0.5f, -0.5f,  0.5f}, {1, 0, 0}, {0.0f, 0.0f}},  //21
                {{ 0.5f,  0.5f, -0.5f}, {1, 0, 0}, {1.0f, 0.0f}},  //22
                {{ 0.5f,  0.5f,  0.5f}, {1, 0, 0}, {0.5f, 1.0f}},  //23

                // Top (+Y)
                {{-0.5f, 0.5f,  0.5f}, {0, 1, 0}, {0.0f, 0.0f}},   //24
                {{ 0.5f, 0.5f,  0.5f}, {0, 1, 0}, {1.0f, 0.0f}},   //25
                {{ 0.5f, 0.5f, -0.5f}, {0, 1, 0}, {0.5f, 1.0f}},   //26
                {{-0.5f, 0.5f,  0.5f}, {0, 1, 0}, {0.0f, 0.0f}},   //27
                {{ 0.5f, 0.5f, -0.5f}, {0, 1, 0}, {1.0f, 0.0f}},   //28
                {{-0.5f, 0.5f, -0.5f}, {0, 1, 0}, {0.5f, 1.0f}},   //29

                // Bottom (-Y)
                {{-0.5f, -0.5f, -0.5f}, {0, -1, 0}, {0.0f, 0.0f}}, //30
                {{ 0.5f, -0.5f, -0.5f}, {0, -1, 0}, {1.0f, 0.0f}}, //31
                {{ 0.5f, -0.5f,  0.5f}, {0, -1, 0}, {0.5f, 1.0f}}, //32
                {{-0.5f, -0.5f, -0.5f}, {0, -1, 0}, {0.0f, 0.0f}}, //33
                {{ 0.5f, -0.5f,  0.5f}, {0, -1, 0}, {1.0f, 0.0f}}, //34
                {{-0.5f, -0.5f,  0.5f}, {0, -1, 0}, {0.5f, 1.0f}}, //35
            }
        };

        return cube;
    }

    const Mesh& getConePrefab(unsigned int segments)
    {
        static Mesh cone;
        if (!cone.vertices.empty()) return cone;

        const float radius = 0.5f;
        const float height = 1.0f;
        const float halfHeight = height * 0.5f;
        const math::Vec3 apex = {0.0f, halfHeight, 0.0f};
        const math::Vec3 base_center = {0.0f, -halfHeight, 0.0f};

        // Base center vertex
        cone.vertices.push_back({base_center, {0, -1, 0}, {0.5f, 0.5f}});
        unsigned int center_index = 0;

        std::vector<unsigned int> base_ring_indices;

        // Base ring
        for (unsigned int i = 0; i < segments; ++i) {
            float angle = (float)i / segments * math::two_pi<float>();
            float x = cos(angle) * radius;
            float z = sin(angle) * radius;

            math::Vec3 pos = {x, -halfHeight, z};
            math::Vec2 uv = {x * 0.5f + 0.5f, z * 0.5f + 0.5f};

            // Flat normal for the base
            cone.vertices.push_back({pos, {0, -1, 0}, uv});
            base_ring_indices.push_back(cone.vertices.size() - 1);
        }

        // Base indices (triangle fan, CCW from bottom view)
        for (unsigned int i = 0; i < segments; ++i) {
            unsigned int next = (i + 1) % segments;
            cone.indices.push_back(center_index);
            cone.indices.push_back(base_ring_indices[i]);
            cone.indices.push_back(base_ring_indices[next]);
        }

        // Side vertices with smooth normals
        unsigned int base_offset = cone.vertices.size();

        for (unsigned int i = 0; i < segments; ++i) {
            float angle = (float)i / segments * math::two_pi<float>();
            float nextAngle = (float)(i + 1) / segments * math::two_pi<float>();

            // Compute side positions
            math::Vec3 base1 = {cos(angle) * radius, -halfHeight, sin(angle) * radius};
            math::Vec3 base2 = {cos(nextAngle) * radius, -halfHeight, sin(nextAngle) * radius};

            // Direction from base ring point to apex
            math::Vec3 dir1 = math::normalize(apex - base1);
            math::Vec3 dir2 = math::normalize(apex - base2);

            // Per-vertex normal: perpendicular to the surface
            math::Vec3 sideNormal1 = math::normalize(math::cross(math::cross(dir1, {0, 1, 0}), dir1));
            math::Vec3 sideNormal2 = math::normalize(math::cross(math::cross(dir2, {0, 1, 0}), dir2));

            // Apex duplicated per triangle with averaged normal
            math::Vec3 apexNormal = math::normalize(sideNormal1 + sideNormal2);

            // Build triangle (correct CCW winding when viewed from outside)
            unsigned int i0 = cone.vertices.size(); // base1
            unsigned int i1 = i0 + 1;               // base2
            unsigned int i2 = i0 + 2;               // apex

            cone.vertices.push_back({base1, sideNormal1, {0, 0}});
            cone.vertices.push_back({base2, sideNormal2, {1, 0}});
            cone.vertices.push_back({apex, apexNormal, {0.5f, 1.0f}});

            // Swap i1 and i0 to enforce CCW
            cone.indices.insert(cone.indices.end(), {i1, i0, i2});
        }

        return cone;
    }

    const Mesh& getCylinderPrefab(unsigned int segments)
    {
        static Mesh cyl;
        if (!cyl.vertices.empty()) return cyl;

        float radius = 0.5f, height = 1.0f;
        float y_top = +height * 0.5f, y_bottom = -height * 0.5f;

        // ---------- Top cap ----------
        unsigned int top_center_idx = cyl.vertices.size();
        cyl.vertices.push_back({{0, y_top, 0}, {0, 1, 0}, {0.5f, 0.5f}});

        unsigned int top_ring_start = cyl.vertices.size();
        for (unsigned int i = 0; i < segments; ++i) {
            float angle = (float)i / segments * math::two_pi<float>();
            float x = cos(angle) * radius;
            float z = sin(angle) * radius;
            math::Vec2 uv = {x * 0.5f + 0.5f, z * 0.5f + 0.5f};
            cyl.vertices.push_back({{x, y_top, z}, {0, 1, 0}, uv});
        }

        for (unsigned int i = 0; i < segments; ++i) {
            unsigned int next = (i + 1) % segments;
            // CCW winding
            cyl.indices.insert(cyl.indices.end(), {
                top_center_idx, top_ring_start + next, top_ring_start + i
            });
        }

        // ---------- Bottom cap ----------
        unsigned int bottom_center_idx = cyl.vertices.size();
        cyl.vertices.push_back({{0, y_bottom, 0}, {0, -1, 0}, {0.5f, 0.5f}});

        unsigned int bottom_ring_start = cyl.vertices.size();
        for (unsigned int i = 0; i < segments; ++i) {
            float angle = (float)i / segments * math::two_pi<float>();
            float x = cos(angle) * radius;
            float z = sin(angle) * radius;
            math::Vec2 uv = {x * 0.5f + 0.5f, z * 0.5f + 0.5f};
            cyl.vertices.push_back({{x, y_bottom, z}, {0, -1, 0}, uv});
        }

        for (unsigned int i = 0; i < segments; ++i) {
            unsigned int next = (i + 1) % segments;
            // CCW winding when viewed from below
            cyl.indices.insert(cyl.indices.end(), {
                bottom_center_idx, bottom_ring_start + i, bottom_ring_start + next
            });
        }

        // ---------- Side wall (smooth normals) ----------
        unsigned int side_start = cyl.vertices.size();
        for (unsigned int i = 0; i <= segments; ++i) {
            float angle = (float)i / segments * math::two_pi<float>();
            float x = cos(angle) * radius;
            float z = sin(angle) * radius;
            math::Vec3 normal = math::normalize(math::Vec3(x, 0.0f, z));
            float u = float(i) / segments;

            cyl.vertices.push_back({{x, y_top, z}, normal, {u, 0.0f}});
            cyl.vertices.push_back({{x, y_bottom, z}, normal, {u, 1.0f}});
        }

        for (unsigned int i = 0; i < segments; ++i) {
            unsigned int i0 = side_start + i * 2;
            unsigned int i1 = i0 + 1;
            unsigned int i2 = i0 + 2;
            unsigned int i3 = i0 + 3;
            // CCW winding
            cyl.indices.insert(cyl.indices.end(), {
                i0, i2, i1,
                i2, i3, i1
            });
        }

        return cyl;
    }

    const Mesh& getIcoSpherePrefab(unsigned int subdivisions, TriangleWinding winding, bool inward_normals)
    {
        static std::vector<Mesh> cache;
        if (cache.size() <= subdivisions) cache.resize(subdivisions + 1);
        if (!cache[subdivisions].vertices.empty()) return cache[subdivisions];

        auto& mesh = cache[subdivisions];

        // Golden ratio
        const float t = (1.0f + math::sqrt(5.0f)) / 2.0f;

        std::vector<math::Vec3> positions = {
            {-1,  t,  0}, { 1,  t,  0}, {-1, -t,  0}, { 1, -t,  0},
            { 0, -1,  t}, { 0,  1,  t}, { 0, -1, -t}, { 0,  1, -t},
            { t,  0, -1}, { t,  0,  1}, {-t,  0, -1}, {-t,  0,  1}
        };

        for (auto& p : positions)
            p = math::normalize(p);

        std::vector<std::array<uint32_t, 3>> faces = {
            {0, 11, 5}, {0, 5, 1}, {0, 1, 7}, {0, 7, 10}, {0, 10, 11},
            {1, 5, 9}, {5, 11, 4}, {11, 10, 2}, {10, 7, 6}, {7, 1, 8},
            {3, 9, 4}, {3, 4, 2}, {3, 2, 6}, {3, 6, 8}, {3, 8, 9},
            {4, 9, 5}, {2, 4, 11}, {6, 2, 10}, {8, 6, 7}, {9, 8, 1}
        };

        absl::flat_hash_map<std::pair<uint32_t, uint32_t>, uint32_t, detail::VertexHasher> midpoint_cache;

        auto getMiddlePoint = [&](const uint32_t a, const uint32_t b) -> uint32_t {
            auto key = std::minmax(a, b);
            auto it = midpoint_cache.find(key);
            if (it != midpoint_cache.end())
                return it->second;

            math::Vec3 midpoint = math::normalize((positions[a] + positions[b]) * 0.5f);
            positions.push_back(midpoint);
            uint32_t idx = static_cast<uint32_t>(positions.size() - 1);
            midpoint_cache[key] = idx;
            return idx;
        };

        for (int i = 0; i < subdivisions; ++i) {
            std::vector<std::array<uint32_t, 3>> subdivided;
            for (const auto& tri : faces) {
                const uint32_t a = getMiddlePoint(tri[0], tri[1]);
                const uint32_t b = getMiddlePoint(tri[1], tri[2]);
                const uint32_t c = getMiddlePoint(tri[2], tri[0]);

                auto addTriangle = [&](uint32_t i0, uint32_t i1, uint32_t i2) {
                    if (winding == TriangleWinding::Clockwise) 
                        subdivided.push_back({i0, i2, i1});
                    else
                        subdivided.push_back({i0, i1, i2});
                };

                addTriangle(tri[0], a, c);
                addTriangle(tri[1], b, a);
                addTriangle(tri[2], c, b);
                addTriangle(a, b, c);

            }
            faces = std::move(subdivided);
        }

        // Deduplicate vertices and apply winding
        std::vector<uint32_t> remap(positions.size(), -1);
        for (const auto& tri : faces) {
            std::array<uint32_t, 3> ordered = tri;
            if (winding == TriangleWinding::Clockwise) {
                std::swap(ordered[1], ordered[2]);
            }

            for (int j = 0; j < 3; ++j) {
                uint32_t idx = ordered[j];
                if (remap[idx] == uint32_t(-1)) {
                    math::Vec3 pos = positions[idx];
                    math::Vec3 normal = inward_normals ? -pos : pos;
                    remap[idx] = static_cast<uint32_t>(mesh.vertices.size());
                    mesh.vertices.push_back({pos, normal, {0, 0}});
                }
                mesh.indices.push_back(remap[idx]);
            }
        }

        return mesh;
    }
}