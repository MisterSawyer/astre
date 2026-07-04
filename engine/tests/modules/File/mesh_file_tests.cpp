#include <gtest/gtest.h>
#include <fstream>
#include "file/file.hpp"

// Minimal OBJ round-trip: a single triangle read back through assimp must yield
// 3 vertices and 3 indices, named by the file stem.
TEST(MeshFileTest, ReadsSimpleObjTriangle) {
    const std::filesystem::path dir = std::filesystem::current_path() / "mesh_file_test_data";
    std::filesystem::create_directories(dir);
    const std::filesystem::path obj = dir / "tri.obj";

    {
        std::ofstream out(obj);
        out << "v 0.0 0.0 0.0\n"
               "v 1.0 0.0 0.0\n"
               "v 0.0 1.0 0.0\n"
               "vt 0.0 0.0\n"
               "vt 1.0 0.0\n"
               "vt 0.0 1.0\n"
               "f 1/1 2/2 3/3\n";
    }

    astre::file::MeshFile mesh_file;
    auto def = mesh_file.read(obj);

    ASSERT_TRUE(def.has_value());
    EXPECT_EQ(def->name(), "tri");
    EXPECT_EQ(def->vertices_size(), 3);
    EXPECT_EQ(def->indices_size(), 3);

    EXPECT_FALSE(mesh_file.read(dir / "missing.obj").has_value());

    std::filesystem::remove_all(dir);
}
