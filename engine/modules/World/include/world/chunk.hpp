#pragma once
#include <math/math.hpp>

namespace astre::world
{
    struct ChunkID 
    {
        int x;
        int y;
        int z;

        static ChunkID fromPosition(const math::Vec3 & position, float chunk_size);
    };

    template <typename H>
    inline H AbslHashValue(H h, const ChunkID & id) {
        return H::combine(std::move(h), id.x, id.y, id.z);
    }

    inline bool operator==(const ChunkID & lhs, const ChunkID & rhs){
        return lhs.x == rhs.x && lhs.y == rhs.y && lhs.z == rhs.z;
    }
}


