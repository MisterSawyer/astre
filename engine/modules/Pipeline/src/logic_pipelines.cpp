#include "pipeline/logic_pipelines.hpp"

namespace astre::pipeline
{
    asio::awaitable<void> runECS(ecs::Systems & systems, float dt, render::Frame & render_frame)
    {
        auto ex = co_await asio::this_coro::executor;
        const auto no_cancel = asio::bind_cancellation_slot(asio::cancellation_slot{}, asio::use_awaitable);

        // --- Transform + Input in parallel
        {
            auto g = asio::experimental::make_parallel_group(
                asio::co_spawn(ex, systems.transform.run(dt), asio::deferred),
                asio::co_spawn(ex, systems.input.run(dt), asio::deferred)
            );

            auto [ord, e1, e2] =
                co_await g.async_wait(asio::experimental::wait_for_all(), no_cancel);

            if (e1) std::rethrow_exception(e1);
            if (e2) std::rethrow_exception(e2);
        }

        // Serial steps
        systems.script.run(dt);
        systems.camera.run(dt, render_frame);

        // --- Visual + Light in parallel
        {
            auto g = asio::experimental::make_parallel_group(
                asio::co_spawn(ex, systems.visual.run(dt, render_frame), asio::deferred),
                asio::co_spawn(ex, systems.light.run(dt,  render_frame), asio::deferred)
            );

            auto [ord, e1, e2] =
                co_await g.async_wait(asio::experimental::wait_for_all(), no_cancel);

            if (e1) std::rethrow_exception(e1);
            if (e2) std::rethrow_exception(e2);
        }

        co_return;
    }
}