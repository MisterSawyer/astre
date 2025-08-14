#include <spdlog/spdlog.h>

#include "input/input.hpp"

namespace astre::input
{
    InputCode keyToInputCode(const std::string & key_name)
    {
        // Letters
        if(key_name == "KEY_A") return InputCode::KEY_A;
        if(key_name == "KEY_B") return InputCode::KEY_B;
        if(key_name == "KEY_C") return InputCode::KEY_C;
        if(key_name == "KEY_D") return InputCode::KEY_D;
        if(key_name == "KEY_E") return InputCode::KEY_E;
        if(key_name == "KEY_F") return InputCode::KEY_F;
        if(key_name == "KEY_G") return InputCode::KEY_G;
        if(key_name == "KEY_H") return InputCode::KEY_H;
        if(key_name == "KEY_I") return InputCode::KEY_I;
        if(key_name == "KEY_J") return InputCode::KEY_J;
        if(key_name == "KEY_K") return InputCode::KEY_K;
        if(key_name == "KEY_L") return InputCode::KEY_L;
        if(key_name == "KEY_M") return InputCode::KEY_M;
        if(key_name == "KEY_N") return InputCode::KEY_N;
        if(key_name == "KEY_O") return InputCode::KEY_O;
        if(key_name == "KEY_P") return InputCode::KEY_P;
        if(key_name == "KEY_Q") return InputCode::KEY_Q;
        if(key_name == "KEY_R") return InputCode::KEY_R;
        if(key_name == "KEY_S") return InputCode::KEY_S;
        if(key_name == "KEY_T") return InputCode::KEY_T;
        if(key_name == "KEY_U") return InputCode::KEY_U;
        if(key_name == "KEY_V") return InputCode::KEY_V;
        if(key_name == "KEY_W") return InputCode::KEY_W;
        if(key_name == "KEY_X") return InputCode::KEY_X;
        if(key_name == "KEY_Y") return InputCode::KEY_Y;
        if(key_name == "KEY_Z") return InputCode::KEY_Z;
        
        // Numbers (Top row)
        if(key_name == "KEY_0") return InputCode::KEY_0;
        if(key_name == "KEY_1") return InputCode::KEY_1;
        if(key_name == "KEY_2") return InputCode::KEY_2;
        if(key_name == "KEY_3") return InputCode::KEY_3;
        if(key_name == "KEY_4") return InputCode::KEY_4;
        if(key_name == "KEY_5") return InputCode::KEY_5;
        if(key_name == "KEY_6") return InputCode::KEY_6;
        if(key_name == "KEY_7") return InputCode::KEY_7;
        if(key_name == "KEY_8") return InputCode::KEY_8;
        if(key_name == "KEY_9") return InputCode::KEY_9;
        
        // Function keys
        if(key_name == "KEY_F1") return InputCode::KEY_F1;
        if(key_name == "KEY_F2") return InputCode::KEY_F2;
        if(key_name == "KEY_F3") return InputCode::KEY_F3;
        if(key_name == "KEY_F4") return InputCode::KEY_F4;
        if(key_name == "KEY_F5") return InputCode::KEY_F5;
        if(key_name == "KEY_F6") return InputCode::KEY_F6;
        if(key_name == "KEY_F7") return InputCode::KEY_F7;
        if(key_name == "KEY_F8") return InputCode::KEY_F8;
        if(key_name == "KEY_F9") return InputCode::KEY_F9;
        if(key_name == "KEY_F10") return InputCode::KEY_F10;
        if(key_name == "KEY_F11") return InputCode::KEY_F11;
        if(key_name == "KEY_F12") return InputCode::KEY_F12;
        
        // Arrows
        if(key_name == "KEY_UP") return InputCode::KEY_UP;
        if(key_name == "KEY_DOWN") return InputCode::KEY_DOWN;
        if(key_name == "KEY_LEFT") return InputCode::KEY_LEFT;
        if(key_name == "KEY_RIGHT") return InputCode::KEY_RIGHT;
        
        // Modifiers - with L/R distinction
        if(key_name == "KEY_LSHIFT") return InputCode::KEY_LSHIFT;
        if(key_name == "KEY_RSHIFT") return InputCode::KEY_RSHIFT;
        if(key_name == "KEY_LCTRL") return InputCode::KEY_LCTRL;
        if(key_name == "KEY_RCTRL") return InputCode::KEY_RCTRL;
        if(key_name == "KEY_LALT") return InputCode::KEY_LALT;
        if(key_name == "KEY_RALT") return InputCode::KEY_RALT;

        if(key_name == "KEY_SPACE") return InputCode::KEY_SPACE;
        if(key_name == "KEY_TAB") return InputCode::KEY_TAB;
        if(key_name == "KEY_ENTER") return InputCode::KEY_ENTER;
        if(key_name == "KEY_ESCAPE") return InputCode::KEY_ESCAPE;
        if(key_name == "KEY_BACKSPACE") return InputCode::KEY_BACKSPACE;
        
        // Special
        if(key_name == "KEY_CAPSLOCK") return InputCode::KEY_CAPSLOCK;
        if(key_name == "KEY_INSERT") return InputCode::KEY_INSERT;
        if(key_name == "KEY_DELETE") return InputCode::KEY_DELETE;
        if(key_name == "KEY_HOME") return InputCode::KEY_HOME;
        if(key_name == "KEY_END") return InputCode::KEY_END;
        if(key_name == "KEY_PAGEUP") return InputCode::KEY_PAGEUP;
        if(key_name == "KEY_PAGEDOWN") return InputCode::KEY_PAGEDOWN;
        
        // Numpad
        if(key_name == "KEY_NUMPAD_0") return InputCode::KEY_NUMPAD_0;
        if(key_name == "KEY_NUMPAD_1") return InputCode::KEY_NUMPAD_1;
        if(key_name == "KEY_NUMPAD_2") return InputCode::KEY_NUMPAD_2;
        if(key_name == "KEY_NUMPAD_3") return InputCode::KEY_NUMPAD_3;
        if(key_name == "KEY_NUMPAD_4") return InputCode::KEY_NUMPAD_4;
        if(key_name == "KEY_NUMPAD_5") return InputCode::KEY_NUMPAD_5;
        if(key_name == "KEY_NUMPAD_6") return InputCode::KEY_NUMPAD_6;
        if(key_name == "KEY_NUMPAD_7") return InputCode::KEY_NUMPAD_7;
        if(key_name == "KEY_NUMPAD_8") return InputCode::KEY_NUMPAD_8;
        if(key_name == "KEY_NUMPAD_9") return InputCode::KEY_NUMPAD_9;
        if(key_name == "KEY_NUMPAD_ADD") return InputCode::KEY_NUMPAD_ADD;
        if(key_name == "KEY_NUMPAD_SUB") return InputCode::KEY_NUMPAD_SUB;
        if(key_name == "KEY_NUMPAD_MUL") return InputCode::KEY_NUMPAD_MUL;
        if(key_name == "KEY_NUMPAD_DIV") return InputCode::KEY_NUMPAD_DIV;
        if(key_name == "KEY_NUMPAD_ENTER") return InputCode::KEY_NUMPAD_ENTER;
            
        // Mouse buttons
        if(key_name == "MOUSE_LEFT") return InputCode::MOUSE_LEFT;
        if(key_name == "MOUSE_RIGHT") return InputCode::MOUSE_RIGHT;
        if(key_name == "MOUSE_MIDDLE") return InputCode::MOUSE_MIDDLE;
        if(key_name == "MOUSE_X1") return InputCode::MOUSE_X1;
        if(key_name == "MOUSE_X2") return InputCode::MOUSE_X2;

        return InputCode::UNKNOWN_InputCode;
    }

    InputService::InputService(process::IProcess & process)
    :   _input_context(process.getExecutionContext())
    {}

    asio::awaitable<void> InputService::recordKeyPressed(async::LifecycleToken & token, InputCode key)
    {
        co_stop_if(token);

        InputEvent event;
        event.set_type(InputEventType::Pressed);
        event.set_code(key);

        co_await _input_context.ensureOnStrand();
        
        _event_queue.emplace_back(std::move(event));
        co_return;
    }

    asio::awaitable<void> InputService::recordKeyReleased(async::LifecycleToken & token, InputCode key)
    {
        co_stop_if(token);

        InputEvent event;
        event.set_type(InputEventType::Released);
        event.set_code(key);

        co_await _input_context.ensureOnStrand();

        _event_queue.emplace_back(std::move(event));
        co_return;
    }

    asio::awaitable<void> InputService::recordMouseMoved(async::LifecycleToken & token, float x, float y, float dx, float dy)
    {
        co_stop_if(token);

        InputEvent event;
        event.set_type(InputEventType::MouseMove);
        event.mutable_mouse()->set_x(x);
        event.mutable_mouse()->set_y(y);
        event.mutable_mouse()->set_dx(dx);
        event.mutable_mouse()->set_dy(dy);

        co_await _input_context.ensureOnStrand();

        _event_queue.emplace_back(std::move(event));
        co_return;
    }

    bool InputService::isKeyHeld(InputCode key) const
    {
        return isInputPresent(key, _held_keys);
    }

    bool InputService::isKeyJustPressed(InputCode key) const
    {
        return isInputPresent(key, _just_pressed);
    }

    bool InputService::isKeyJustReleased(InputCode key) const
    {
        return isInputPresent(key, _just_released);
    }

    asio::awaitable<void> InputService::update(async::LifecycleToken & token)
    {
        co_stop_if(token);
        
        std::deque<InputEvent> events;

        co_await _input_context.ensureOnStrand();
        _just_pressed.clear();
        _just_released.clear();
        _mouse_delta.x = 0.0f;
        _mouse_delta.y = 0.0f;

        std::swap(events, _event_queue); // fast, av

        for (const auto& event : events)
        {
            switch (event.type()) 
            {
                case InputEventType::Pressed:
                    if(event.code() == InputCode::UNKNOWN_InputCode) break;
                    if (_held_keys.insert(event.code()).second)
                    {
                        _just_pressed.insert(event.code());
                    }
                    break;
                case InputEventType::Released:
                    if(event.code() == InputCode::UNKNOWN_InputCode) break;
                    if (_held_keys.erase(event.code()) > 0)
                    {
                        _just_released.insert(event.code());
                    }
                    break;
                case InputEventType::MouseMove:
                    _mouse_pos.x = event.mouse().x();
                    _mouse_pos.y = event.mouse().y();
                    _mouse_delta.x += event.mouse().dx();
                    _mouse_delta.y += event.mouse().dy();
                    break;
                default:
                    break;
            }
        }
    }

}