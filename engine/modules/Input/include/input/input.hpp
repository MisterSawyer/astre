#pragma once

#include <atomic>
#include <queue>
#include <deque>

#include "native/native.h"
#include <asio.hpp>
#include <absl/container/flat_hash_set.h>

#include "async/async.hpp"
#include "process/process.hpp"

#include "generated/Input/proto/input_event.pb.h"

namespace astre::input
{
    InputCode keyToInputCode(int key_code);
    InputCode keyToInputCode(const std::string & key_name);

    template<class T>
    inline bool isInputPresent(InputCode code, const T & keys_set)
    {
        return std::find(keys_set.begin(), keys_set.end(), code) != keys_set.end();
    }

    class InputService
    {
        public:
            InputService(process::IProcess & process);
            InputService(const InputService&) = delete;
            InputService(InputService&&) = delete;
            InputService& operator=(const InputService&) = delete;
            InputService& operator=(InputService&&) = delete;
            ~InputService() = default;

            asio::awaitable<void> recordKeyPressed(async::LifecycleToken & token, InputCode key);

            asio::awaitable<void> recordKeyReleased(async::LifecycleToken & token, InputCode key);

            asio::awaitable<void> recordMouseMoved(async::LifecycleToken & token, float x, float y, float dx, float dy);

            bool isKeyPressed(InputCode key) const;

            const absl::flat_hash_set<InputCode> & getPressed() const { return _held_keys; }
            const absl::flat_hash_set<InputCode> & getJustPressed() const { return _just_pressed; }
            const absl::flat_hash_set<InputCode> & getJustReleased() const { return _just_released; }

            asio::awaitable<void> update(async::LifecycleToken & token);

        private:
            async::AsyncContext<process::IProcess::execution_context_type> _input_context;

            std::deque<InputEvent> _event_queue;
            absl::flat_hash_set<InputCode> _held_keys;

            absl::flat_hash_set<InputCode> _just_pressed;
            absl::flat_hash_set<InputCode> _just_released;
    };

}