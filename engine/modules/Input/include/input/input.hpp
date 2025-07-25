#pragma once

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
    constexpr inline bool isInputPresent(InputCode code, const T & keys_set)
    {
        return std::find(keys_set.begin(), keys_set.end(), code) != keys_set.end();
    }

    class InputService
    {
        public:
            InputService(async::AsyncContext<process::IProcess::execution_context_type> & ctx);
            InputService(const InputService&) = delete;
            InputService(InputService&&) = delete;
            InputService& operator=(const InputService&) = delete;
            InputService& operator=(InputService&&) = delete;
            ~InputService() = default;

            asio::awaitable<void> recordKeyPressed(InputCode key);

            asio::awaitable<void> recordKeyReleased(InputCode key);

            asio::awaitable<void> recordMouseMoved(float x, float y, float dx, float dy);

        private:

            async::AsyncContext<process::IProcess::execution_context_type> & _input_context;

            std::deque<InputEvent> _event_queue;
            absl::flat_hash_set<InputCode> _held_keys;
    };

}