#include <astre.hpp>

namespace astre::entry
{
    asio::awaitable<int> main(asio::thread_pool & thread_pool, process::IProcess & process)
    {
        spdlog::info("astre game started");


        co_return 0;
    }
}