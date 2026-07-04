#include "file/script_file.hpp"

#include <fstream>
#include <sstream>
#include <string>

#include <spdlog/spdlog.h>

namespace astre::file
{
    std::optional<proto::script::ScriptDefinition> ScriptFile::read(const std::filesystem::path & file) const
    {
        const std::string name = file.stem().string();
        spdlog::debug("[script-file] Reading script from file {}", file.string());

        std::ifstream in(file);
        if(!in.is_open())
        {
            spdlog::error("Failed to open lua script: {}", file.string());
            return std::nullopt;
        }

        std::stringstream buffer;
        buffer << in.rdbuf();

        proto::script::ScriptDefinition def;
        def.set_name(name);
        def.add_code(buffer.str());

        return def;
    }
}
