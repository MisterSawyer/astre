#include <spdlog/spdlog.h>

#include "input/input.hpp"

namespace astre::input
{
    proto::input::InputCode keyToInputCode(const std::string & key_name)
    {
        // Letters
        if(key_name == "KEY_A") return proto::input::InputCode::KEY_A;
        if(key_name == "KEY_B") return proto::input::InputCode::KEY_B;
        if(key_name == "KEY_C") return proto::input::InputCode::KEY_C;
        if(key_name == "KEY_D") return proto::input::InputCode::KEY_D;
        if(key_name == "KEY_E") return proto::input::InputCode::KEY_E;
        if(key_name == "KEY_F") return proto::input::InputCode::KEY_F;
        if(key_name == "KEY_G") return proto::input::InputCode::KEY_G;
        if(key_name == "KEY_H") return proto::input::InputCode::KEY_H;
        if(key_name == "KEY_I") return proto::input::InputCode::KEY_I;
        if(key_name == "KEY_J") return proto::input::InputCode::KEY_J;
        if(key_name == "KEY_K") return proto::input::InputCode::KEY_K;
        if(key_name == "KEY_L") return proto::input::InputCode::KEY_L;
        if(key_name == "KEY_M") return proto::input::InputCode::KEY_M;
        if(key_name == "KEY_N") return proto::input::InputCode::KEY_N;
        if(key_name == "KEY_O") return proto::input::InputCode::KEY_O;
        if(key_name == "KEY_P") return proto::input::InputCode::KEY_P;
        if(key_name == "KEY_Q") return proto::input::InputCode::KEY_Q;
        if(key_name == "KEY_R") return proto::input::InputCode::KEY_R;
        if(key_name == "KEY_S") return proto::input::InputCode::KEY_S;
        if(key_name == "KEY_T") return proto::input::InputCode::KEY_T;
        if(key_name == "KEY_U") return proto::input::InputCode::KEY_U;
        if(key_name == "KEY_V") return proto::input::InputCode::KEY_V;
        if(key_name == "KEY_W") return proto::input::InputCode::KEY_W;
        if(key_name == "KEY_X") return proto::input::InputCode::KEY_X;
        if(key_name == "KEY_Y") return proto::input::InputCode::KEY_Y;
        if(key_name == "KEY_Z") return proto::input::InputCode::KEY_Z;
        
        // Numbers (Top row)
        if(key_name == "KEY_0") return proto::input::InputCode::KEY_0;
        if(key_name == "KEY_1") return proto::input::InputCode::KEY_1;
        if(key_name == "KEY_2") return proto::input::InputCode::KEY_2;
        if(key_name == "KEY_3") return proto::input::InputCode::KEY_3;
        if(key_name == "KEY_4") return proto::input::InputCode::KEY_4;
        if(key_name == "KEY_5") return proto::input::InputCode::KEY_5;
        if(key_name == "KEY_6") return proto::input::InputCode::KEY_6;
        if(key_name == "KEY_7") return proto::input::InputCode::KEY_7;
        if(key_name == "KEY_8") return proto::input::InputCode::KEY_8;
        if(key_name == "KEY_9") return proto::input::InputCode::KEY_9;
        
        // Function keys
        if(key_name == "KEY_F1") return proto::input::InputCode::KEY_F1;
        if(key_name == "KEY_F2") return proto::input::InputCode::KEY_F2;
        if(key_name == "KEY_F3") return proto::input::InputCode::KEY_F3;
        if(key_name == "KEY_F4") return proto::input::InputCode::KEY_F4;
        if(key_name == "KEY_F5") return proto::input::InputCode::KEY_F5;
        if(key_name == "KEY_F6") return proto::input::InputCode::KEY_F6;
        if(key_name == "KEY_F7") return proto::input::InputCode::KEY_F7;
        if(key_name == "KEY_F8") return proto::input::InputCode::KEY_F8;
        if(key_name == "KEY_F9") return proto::input::InputCode::KEY_F9;
        if(key_name == "KEY_F10") return proto::input::InputCode::KEY_F10;
        if(key_name == "KEY_F11") return proto::input::InputCode::KEY_F11;
        if(key_name == "KEY_F12") return proto::input::InputCode::KEY_F12;
        
        // Arrows
        if(key_name == "KEY_UP") return proto::input::InputCode::KEY_UP;
        if(key_name == "KEY_DOWN") return proto::input::InputCode::KEY_DOWN;
        if(key_name == "KEY_LEFT") return proto::input::InputCode::KEY_LEFT;
        if(key_name == "KEY_RIGHT") return proto::input::InputCode::KEY_RIGHT;
        
        // Modifiers - with L/R distinction
        if(key_name == "KEY_LSHIFT") return proto::input::InputCode::KEY_LSHIFT;
        if(key_name == "KEY_RSHIFT") return proto::input::InputCode::KEY_RSHIFT;
        if(key_name == "KEY_LCTRL") return proto::input::InputCode::KEY_LCTRL;
        if(key_name == "KEY_RCTRL") return proto::input::InputCode::KEY_RCTRL;
        if(key_name == "KEY_LALT") return proto::input::InputCode::KEY_LALT;
        if(key_name == "KEY_RALT") return proto::input::InputCode::KEY_RALT;

        if(key_name == "KEY_SPACE") return proto::input::InputCode::KEY_SPACE;
        if(key_name == "KEY_TAB") return proto::input::InputCode::KEY_TAB;
        if(key_name == "KEY_ENTER") return proto::input::InputCode::KEY_ENTER;
        if(key_name == "KEY_ESCAPE") return proto::input::InputCode::KEY_ESCAPE;
        if(key_name == "KEY_BACKSPACE") return proto::input::InputCode::KEY_BACKSPACE;
        
        // Special
        if(key_name == "KEY_CAPSLOCK") return proto::input::InputCode::KEY_CAPSLOCK;
        if(key_name == "KEY_INSERT") return proto::input::InputCode::KEY_INSERT;
        if(key_name == "KEY_DELETE") return proto::input::InputCode::KEY_DELETE;
        if(key_name == "KEY_HOME") return proto::input::InputCode::KEY_HOME;
        if(key_name == "KEY_END") return proto::input::InputCode::KEY_END;
        if(key_name == "KEY_PAGEUP") return proto::input::InputCode::KEY_PAGEUP;
        if(key_name == "KEY_PAGEDOWN") return proto::input::InputCode::KEY_PAGEDOWN;
        
        // Numpad
        if(key_name == "KEY_NUMPAD_0") return proto::input::InputCode::KEY_NUMPAD_0;
        if(key_name == "KEY_NUMPAD_1") return proto::input::InputCode::KEY_NUMPAD_1;
        if(key_name == "KEY_NUMPAD_2") return proto::input::InputCode::KEY_NUMPAD_2;
        if(key_name == "KEY_NUMPAD_3") return proto::input::InputCode::KEY_NUMPAD_3;
        if(key_name == "KEY_NUMPAD_4") return proto::input::InputCode::KEY_NUMPAD_4;
        if(key_name == "KEY_NUMPAD_5") return proto::input::InputCode::KEY_NUMPAD_5;
        if(key_name == "KEY_NUMPAD_6") return proto::input::InputCode::KEY_NUMPAD_6;
        if(key_name == "KEY_NUMPAD_7") return proto::input::InputCode::KEY_NUMPAD_7;
        if(key_name == "KEY_NUMPAD_8") return proto::input::InputCode::KEY_NUMPAD_8;
        if(key_name == "KEY_NUMPAD_9") return proto::input::InputCode::KEY_NUMPAD_9;
        if(key_name == "KEY_NUMPAD_ADD") return proto::input::InputCode::KEY_NUMPAD_ADD;
        if(key_name == "KEY_NUMPAD_SUB") return proto::input::InputCode::KEY_NUMPAD_SUB;
        if(key_name == "KEY_NUMPAD_MUL") return proto::input::InputCode::KEY_NUMPAD_MUL;
        if(key_name == "KEY_NUMPAD_DIV") return proto::input::InputCode::KEY_NUMPAD_DIV;
        if(key_name == "KEY_NUMPAD_ENTER") return proto::input::InputCode::KEY_NUMPAD_ENTER;
            
        // Mouse buttons
        if(key_name == "MOUSE_LEFT") return proto::input::InputCode::MOUSE_LEFT;
        if(key_name == "MOUSE_RIGHT") return proto::input::InputCode::MOUSE_RIGHT;
        if(key_name == "MOUSE_MIDDLE") return proto::input::InputCode::MOUSE_MIDDLE;
        if(key_name == "MOUSE_X1") return proto::input::InputCode::MOUSE_X1;
        if(key_name == "MOUSE_X2") return proto::input::InputCode::MOUSE_X2;

        return proto::input::InputCode::UNKNOWN_InputCode;
    }

    InputService::InputService(process::IProcess & process, async::LifecycleToken & token)
    :   _input_context(process.getExecutionContext()), _token(token)
    {}

    asio::awaitable<void> InputService::recordKeyPressed(proto::input::InputCode key)
    {
        co_stop_if(_token);

        proto::input::InputEvent event;
        event.set_type(proto::input::InputEventType::Pressed);
        event.set_code(key);

        co_await _input_context.ensureOnStrand();
        
        _event_queue.emplace_back(std::move(event));
        co_return;
    }

    asio::awaitable<void> InputService::recordKeyReleased(proto::input::InputCode key)
    {
        co_stop_if(_token);

        proto::input::InputEvent event;
        event.set_type(proto::input::InputEventType::Released);
        event.set_code(key);

        co_await _input_context.ensureOnStrand();

        _event_queue.emplace_back(std::move(event));
        co_return;
    }

    asio::awaitable<void> InputService::recordMouseMoved(float x, float y, float dx, float dy)
    {
        co_stop_if(_token);

        proto::input::InputEvent event;
        event.set_type(proto::input::InputEventType::MouseMove);
        event.mutable_mouse()->set_x(x);
        event.mutable_mouse()->set_y(y);
        event.mutable_mouse()->set_dx(dx);
        event.mutable_mouse()->set_dy(dy);

        co_await _input_context.ensureOnStrand();

        _event_queue.emplace_back(std::move(event));
        co_return;
    }

    bool InputService::isKeyHeld(proto::input::InputCode key) const
    {
        return isInputPresent(key, _held_keys);
    }

    bool InputService::isKeyJustPressed(proto::input::InputCode key) const
    {
        return isInputPresent(key, _just_pressed);
    }

    bool InputService::isKeyJustReleased(proto::input::InputCode key) const
    {
        return isInputPresent(key, _just_released);
    }

    asio::awaitable<void> InputService::update()
    {
        co_stop_if(_token);
        
        std::deque<proto::input::InputEvent> events;

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
                case proto::input::InputEventType::Pressed:
                    if(event.code() == proto::input::InputCode::UNKNOWN_InputCode) break;
                    if (_held_keys.insert(event.code()).second)
                    {
                        _just_pressed.insert(event.code());
                    }
                    break;
                case proto::input::InputEventType::Released:
                    if(event.code() == proto::input::InputCode::UNKNOWN_InputCode) break;
                    if (_held_keys.erase(event.code()) > 0)
                    {
                        _just_released.insert(event.code());
                    }
                    break;
                case proto::input::InputEventType::MouseMove:
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