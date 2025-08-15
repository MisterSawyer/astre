#pragma once
#include <chrono>

namespace astre::pipeline
{
    class LogicFrameTimer
    {
    public:

        void start()
        {
            _last_time = _start;
            _start = std::chrono::steady_clock::now();
        }

        void end()
        {
            const float frame_time_ms =
                std::chrono::duration<float>(std::chrono::steady_clock::now() - _start).count() * 1000.0f;

            _frame_time = (_frame_time * _frame_smoothing) + (frame_time_ms * (1.0f - _frame_smoothing));
            const float current_fps =
                1.0f / std::max(std::chrono::duration<float>(_start - _last_time).count(), 1e-6f);

            _fps = (_fps * _frame_smoothing) + (current_fps * (1.0f - _frame_smoothing));
        }

        float getFPS() const { return _fps; }
        float getFrameTime() const { return _frame_time; }

    private:

        const float _frame_smoothing = 0.9f;

        std::chrono::steady_clock::time_point _last_time;
        std::chrono::steady_clock::time_point _start;

        float _fps{0.0f};
        float _frame_time{0.0f};
    };
}