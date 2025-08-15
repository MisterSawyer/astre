#include "pipeline/display.hpp"

namespace astre::pipeline
{

    asio::awaitable<std::optional<DisplayResources>> buildDisplayResources(render::IRenderer & renderer, std::pair<unsigned,unsigned> size)
    {
        DisplayResources resources;
        resources.size = std::move(size);
        resources.aspect = (float)resources.size.first / (float)resources.size.second;

        auto viewport_fbo_res = co_await renderer.createFrameBufferObject(
            "fbo::viewport", resources.size,
            {
                {render::FBOAttachment::Type::Texture, render::FBOAttachment::Point::Color, render::TextureFormat::RGB_16F},
            }
        );

        if (!viewport_fbo_res) {
            spdlog::error("Failed to create viewport FBO");
            co_return std::nullopt;
        }

        resources.viewport_fbo = *viewport_fbo_res;

        co_return resources;
    }
}