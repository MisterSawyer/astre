#pragma once

#include <atomic>
#include <queue>
#include <deque>

#include "native/native.h"
#include <asio.hpp>
#include <absl/container/flat_hash_set.h>

#include "math/math.hpp"
#include "async/async.hpp"
#include "process/process.hpp"

#include "proto/Input/input_event.pb.h"

namespace astre::input
{
    proto::input::InputCode keyToInputCode(int key_code);
    proto::input::InputCode keyToInputCode(const std::string & key_name);

    template<class T>
    inline bool isInputPresent(proto::input::InputCode code, const T & keys_set)
    {
        return std::find(keys_set.begin(), keys_set.end(), code) != keys_set.end();
    }

    class InputService
    {
        public:
            InputService(process::IProcess & process, async::LifecycleToken & token);
            InputService(const InputService&) = delete;
            InputService(InputService&&) = delete;
            InputService& operator=(const InputService&) = delete;
            InputService& operator=(InputService&&) = delete;
            ~InputService() = default;

            asio::awaitable<void> recordKeyPressed(proto::input::InputCode key);

            asio::awaitable<void> recordKeyReleased(proto::input::InputCode key);

            asio::awaitable<void> recordMouseMoved(float x, float y, float dx, float dy);

            bool isKeyHeld(proto::input::InputCode key) const;
            bool isKeyJustPressed(proto::input::InputCode key) const;
            bool isKeyJustReleased(proto::input::InputCode key) const;

            const absl::flat_hash_set<proto::input::InputCode> & getHeld() const { return _held_keys; }
            const absl::flat_hash_set<proto::input::InputCode> & getJustPressed() const { return _just_pressed; }
            const absl::flat_hash_set<proto::input::InputCode> & getJustReleased() const { return _just_released; }

            const math::Vec2 & getMousePosition() const { return _mouse_pos; }
            const math::Vec2 & getMouseDelta() const { return _mouse_delta; }

            asio::awaitable<void> update();

        private:
            async::AsyncContext<process::IProcess::execution_context_type> _input_context;

            async::LifecycleToken & _token;

            std::deque<proto::input::InputEvent> _event_queue;

            absl::flat_hash_set<proto::input::InputCode> _held_keys;

            absl::flat_hash_set<proto::input::InputCode> _just_pressed;
            absl::flat_hash_set<proto::input::InputCode> _just_released;

            math::Vec2 _mouse_pos;
            math::Vec2 _mouse_delta;
    };

}