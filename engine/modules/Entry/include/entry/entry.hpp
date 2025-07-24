#pragma once

#include "native/native.h"
#include <asio.hpp>

#include "process/process.hpp"

namespace astre::entry
{
    extern asio::awaitable<int> main(process::IProcess & process);
}

int main(int argc, char** argv);