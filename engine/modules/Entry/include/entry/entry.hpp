#pragma once

#include "native/native.h"
#include <spdlog/spdlog.h>

#include "version/version.h"
#include "process/process.hpp"

namespace astre::entry
{
    extern asio::awaitable<int> main(process::IProcess & process);
}

int main(int argc, char** argv);