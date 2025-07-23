#pragma once

#include <vector>
#include <algorithm>
#include <array>

#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>

#include <absl/container/flat_hash_map.h>

namespace astre::render
{
    //pragma pack(push, 1) to disable automatic padding
    #pragma pack(push, 1)
    /**
     * @brief Vertex
     * 
     */
    struct Vertex 
    {
        glm::vec3 position;
        glm::vec3 normal;
        glm::vec2 uv;
    };
    #pragma pack(pop) // each vertex is now exactly 8 floats = 32 bytes, no extra padding

    /**
     * @brief Mesh
     * 
     */
    struct Mesh
    {
        std::vector<unsigned int> indices;
        std::vector<Vertex> vertices;
    };

    /**
     * @brief Triangle winding
     * 
     */
    enum class TriangleWinding
    {
        Clockwise,
        CounterClockwise
    };

    /**
     * @brief Get the normalized device coordinates quad prefab object.
     * 
     * @return The normalized device coordinates quad prefab object.
     */
    const Mesh & getNormalizedDeviceCoordinatesQuadPrefab();
    
    /**
     * @brief Get the quad prefab object.
     * 
     * @return The quad prefab object.
     */
    const Mesh & getQuadPrefab();

    /**
     * @brief Get the cube prefab object.
     * 
     * @return The cube prefab object.
     */
    const Mesh & getCubePrefab();

    /**
     * @brief Get the sphere prefab object.
     * 
     * @param subdivisions The number of subdivisions.
     * @param winding The winding of the triangles.
     * @return The sphere prefab object.
     */
    const Mesh& getIcoSpherePrefab(unsigned int subdivisions = 1, TriangleWinding winding = TriangleWinding::CounterClockwise, bool inward_normals = false);

    /**
     * @brief Get the cone prefab object.
     * 
     * @param segments The number of segments.
     * @return The cone prefab object.
     */
    const Mesh& getConePrefab(unsigned int segments = 32);

    /**
     * @brief Get the cylinder prefab object.
     * 
     * @param segments The number of segments.
     * @return The cylinder prefab object.
     */
    const Mesh& getCylinderPrefab(unsigned int segments = 32);
}