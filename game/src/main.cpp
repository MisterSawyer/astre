#include <astre.hpp>

namespace astre::entry
{
    asio::awaitable<void> gameMain(const pipeline::GamePipelineState & game_state, const pipeline::WindowAppState & app_state)
    {
        co_await asset::loadVertexBuffersPrefabs(app_state.renderer);

        co_await app_state.renderer.enableVSync();

        async::AsyncContext main_loop_context(app_state.process.getExecutionContext());
        while(app_state.window.good())
        {
            co_await main_loop_context.ensureOnStrand();
            co_await app_state.renderer.clearScreen({1.0f, 1.0f, 0.0f, 1.0f});
            co_await app_state.renderer.present();
            std::this_thread::sleep_for(100ms);
        };
    }

    asio::awaitable<void> appMain(const pipeline::WindowAppState & app_state)
    {
        co_await pipeline::GamePipeline{}.run<void>(gameMain, app_state);
    }

    asio::awaitable<int> main(process::IProcess & process)
    {
        spdlog::set_level(spdlog::level::debug);
        spdlog::info("astre game started");

        co_await pipeline::WindowApp{"astre game", 1280, 720}.run<void>(process, appMain);

        co_return 0;
    }
}