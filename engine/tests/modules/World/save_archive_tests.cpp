#include <gtest/gtest.h>
#include <fstream>
#include "world/world.hpp"

class SaveArchiveTest : public ::testing::Test {
protected:
    std::filesystem::path temp_dir;

    void SetUp() override {
        temp_dir = std::filesystem::current_path() / "save_archive_test_data";
        std::filesystem::create_directories(temp_dir);
    }

    void TearDown() override {
        std::filesystem::remove_all(temp_dir);
    }

    astre::world::WorldChunk createTestChunk(int x, int y, int z, const std::string& name = "test_entity") {
        astre::world::WorldChunk chunk;
        auto* id = chunk.mutable_id();
        id->set_x(x);
        id->set_y(y);
        id->set_z(z);

        auto* entity = chunk.add_entities();
        entity->set_name(name); // Requires name field in EntityDefinition

        return chunk;
    }
};


TEST_F(SaveArchiveTest, WriteAndReadChunk_BinaryFormat) {
    auto chunk = createTestChunk(1, 2, 3, "binary_test");

    std::filesystem::path file = temp_dir / "test_chunk.bin";
    {
        astre::world::SaveArchive writer(file, astre::asset::use_binary);
        ASSERT_TRUE(writer.writeChunk(chunk, astre::asset::use_binary));
    }

    {
        astre::world::SaveArchive reader(file, astre::asset::use_binary);
        auto result = reader.readChunk(chunk.id(), astre::asset::use_binary);
        ASSERT_TRUE(result.has_value());
        EXPECT_EQ(result->id().x(), chunk.id().x());
        EXPECT_EQ(result->entities_size(), 1);
        EXPECT_EQ(result->entities(0).name(), "binary_test");
    }
}


TEST_F(SaveArchiveTest, WriteAndReadChunk_JsonFormat) {
    auto chunk = createTestChunk(4, 5, 6, "json_test");

    std::filesystem::path file = temp_dir / "test_chunk.json";
    {
        astre::world::SaveArchive writer(file, astre::asset::use_json);
        ASSERT_TRUE(writer.writeChunk(chunk, astre::asset::use_json));
    }

    {
        astre::world::SaveArchive reader(file, astre::asset::use_json);
        auto result = reader.readChunk(chunk.id(), astre::asset::use_json);
        ASSERT_TRUE(result.has_value());
        EXPECT_EQ(result->id().x(), chunk.id().x());
        EXPECT_EQ(result->entities_size(), 1);
        EXPECT_EQ(result->entities(0).name(), "json_test");
    }
}


