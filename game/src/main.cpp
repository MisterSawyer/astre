#include <astre.hpp>

namespace astre::entry
{
    asio::awaitable<void> gameMain(const pipeline::GamePipelineState & game_state, const pipeline::WindowAppState & app_state, const entry::AppPaths & paths)
    {
        co_await asset::loadVertexBuffersPrefabs(app_state.renderer);

        async::AsyncContext main_loop_context(app_state.process.getExecutionContext());

        world::WorldStreamer world_streamer{
            main_loop_context,
            paths.resources / "worlds/levels/level0.json",
            game_state.registry, game_state.loader, game_state.serializer, 
            16.0f, 8};
        
        auto entity = game_state.registry.createEntity("player");
        ecs::TransformComponent transform;
        transform.mutable_position()->set_x(17.0f);
        transform.mutable_position()->set_y(11.0f);
        transform.mutable_position()->set_z(3.0f);

        game_state.registry.addComponent<ecs::TransformComponent>(*entity, std::move(transform));
        
        co_await world_streamer.addEntityToChunk(*entity, world::ChunkID{0, 0, 0});

        co_await world_streamer.saveAll(asset::use_json);


        co_await app_state.renderer.enableVSync();
        while(app_state.window.good())
        {
            co_await main_loop_context.ensureOnStrand();
            co_await app_state.renderer.clearScreen({1.0f, 1.0f, 0.0f, 1.0f});
            co_await app_state.renderer.present();
            std::this_thread::sleep_for(100ms);
        };
    }

    asio::awaitable<void> appMain(const pipeline::WindowAppState & app_state, const entry::AppPaths & paths)
    {
        co_await pipeline::GamePipeline{}.run<void>(gameMain, app_state, paths);
    }

    asio::awaitable<int> main(const entry::AppPaths & paths, process::IProcess & process)
    {
        spdlog::set_level(spdlog::level::debug);
        spdlog::info("astre game started");

        co_await pipeline::WindowApp{"astre game", 1280, 720}.run<void>(process, appMain, paths);

        co_return 0;
    }
}