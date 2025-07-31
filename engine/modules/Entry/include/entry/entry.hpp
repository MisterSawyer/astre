#pragma once

#include <filesystem>

#include "native/native.h"
#include <asio.hpp>

#include "process/process.hpp"

namespace astre::entry
{
    struct AppPaths {
        const std::filesystem::path base;
        const std::filesystem::path resources;
        const std::filesystem::path assets;
        const std::filesystem::path saves;
        const std::filesystem::path logs;

        explicit AppPaths(const std::filesystem::path& baseDir);
    };

    extern asio::awaitable<int> main(process::IProcess & process, const AppPaths& paths);
}

int main(int argc, char** argv);