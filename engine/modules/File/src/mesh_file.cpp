#include "file/mesh_file.hpp"

#include <cstdint>

#include <spdlog/spdlog.h>

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include "math/math.hpp"

namespace astre::file
{
    std::optional<proto::render::MeshDefinition> MeshFile::read(const std::filesystem::path & file) const
    {
        const std::string name = file.stem().string();
        spdlog::debug("[mesh-file] Reading mesh from file {}", file.string());

        if(!std::filesystem::exists(file))
        {
            spdlog::error("Mesh file does not exist: {}", file.string());
            return std::nullopt;
        }

        Assimp::Importer importer;
        const aiScene * scene = importer.ReadFile(file.string(),
            aiProcess_Triangulate |
            aiProcess_GenSmoothNormals |
            aiProcess_JoinIdenticalVertices |
            aiProcess_FlipUVs);

        if(scene == nullptr || (scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE) || scene->mRootNode == nullptr)
        {
            spdlog::error("Failed to import mesh {}: {}", file.string(), importer.GetErrorString());
            return std::nullopt;
        }

        proto::render::MeshDefinition mesh_def;
        mesh_def.set_name(name);

        // ponytail: flatten every submesh into one vertex/index buffer; per-material
        //           split lands here when materials do.
        std::uint32_t base = 0;
        for(unsigned int m = 0; m < scene->mNumMeshes; ++m)
        {
            const aiMesh * mesh = scene->mMeshes[m];

            for(unsigned int v = 0; v < mesh->mNumVertices; ++v)
            {
                auto * vert = mesh_def.add_vertices();

                const aiVector3D & p = mesh->mVertices[v];
                *vert->mutable_position() = math::serialize(math::Vec3{p.x, p.y, p.z});

                const aiVector3D n = mesh->mNormals ? mesh->mNormals[v] : aiVector3D(0.0f, 0.0f, 0.0f);
                *vert->mutable_normal() = math::serialize(math::Vec3{n.x, n.y, n.z});

                math::Vec2 uv{0.0f, 0.0f};
                if(mesh->mTextureCoords[0])
                    uv = {mesh->mTextureCoords[0][v].x, mesh->mTextureCoords[0][v].y};
                *vert->mutable_uv() = math::serialize(uv);
            }

            for(unsigned int f = 0; f < mesh->mNumFaces; ++f)
            {
                const aiFace & face = mesh->mFaces[f];
                for(unsigned int i = 0; i < face.mNumIndices; ++i)
                    mesh_def.add_indices(base + face.mIndices[i]);
            }

            base += mesh->mNumVertices;
        }

        if(mesh_def.vertices().empty())
        {
            spdlog::error("Mesh {} contains no vertices", file.string());
            return std::nullopt;
        }

        return mesh_def;
    }
}
