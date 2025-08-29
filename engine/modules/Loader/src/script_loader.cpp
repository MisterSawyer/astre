#include <spdlog/spdlog.h>

#include "asset/asset.hpp"

namespace astre::asset
{
    asio::awaitable<bool> loadScript(script::ScriptRuntime & runtime, const std::filesystem::path path)
    {
        std::ifstream in(path);
        if (!in.is_open()) {
            spdlog::error(std::format("Failed to open lua script: {}", path.string()));
            co_return false;
        }
        std::stringstream buffer;
        buffer << in.rdbuf();

        co_return runtime.loadScript(path.filename().stem().string(), buffer.str());
    }
}