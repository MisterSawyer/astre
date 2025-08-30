
#include "native/native.h"
#include "input/input.hpp"

namespace astre::input::windows
{
    static proto::input::InputCode _keyToInputCode(int key)
    {
            switch (key)
            { 
                // Letters
                case 0x41 : return proto::input::InputCode::KEY_A;
                case 0x42 : return proto::input::InputCode::KEY_B;
                case 0x43 : return proto::input::InputCode::KEY_C;
                case 0x44 : return proto::input::InputCode::KEY_D;
                case 0x45 : return proto::input::InputCode::KEY_E;
                case 0x46 : return proto::input::InputCode::KEY_F;
                case 0x47 : return proto::input::InputCode::KEY_G;
                case 0x48 : return proto::input::InputCode::KEY_H;
                case 0x49 : return proto::input::InputCode::KEY_I;
                case 0x4A : return proto::input::InputCode::KEY_J;
                case 0x4B : return proto::input::InputCode::KEY_K;
                case 0x4C : return proto::input::InputCode::KEY_L;
                case 0x4D : return proto::input::InputCode::KEY_M;
                case 0x4E : return proto::input::InputCode::KEY_N;
                case 0x4F : return proto::input::InputCode::KEY_O;
                case 0x50 : return proto::input::InputCode::KEY_P;
                case 0x51 : return proto::input::InputCode::KEY_Q;
                case 0x52 : return proto::input::InputCode::KEY_R;
                case 0x53 : return proto::input::InputCode::KEY_S;
                case 0x54 : return proto::input::InputCode::KEY_T;
                case 0x55 : return proto::input::InputCode::KEY_U;
                case 0x56 : return proto::input::InputCode::KEY_V;
                case 0x57 : return proto::input::InputCode::KEY_W;
                case 0x58 : return proto::input::InputCode::KEY_X;
                case 0x59 : return proto::input::InputCode::KEY_Y;
                case 0x5A : return proto::input::InputCode::KEY_Z;

                // Numbers (Top rowproto::input::)
                case 0x30 : return proto::input::InputCode::KEY_0;
                case 0x31 : return proto::input::InputCode::KEY_1;
                case 0x32 : return proto::input::InputCode::KEY_2;
                case 0x33 : return proto::input::InputCode::KEY_3;
                case 0x34 : return proto::input::InputCode::KEY_4;
                case 0x35 : return proto::input::InputCode::KEY_5;
                case 0x36 : return proto::input::InputCode::KEY_6;
                case 0x37 : return proto::input::InputCode::KEY_7;
                case 0x38 : return proto::input::InputCode::KEY_8;
                case 0x39 : return proto::input::InputCode::KEY_9;
                
                // Function keys
                case VK_F1 : return proto::input::InputCode::KEY_F1;
                case VK_F2 : return proto::input::InputCode::KEY_F2;
                case VK_F3 : return proto::input::InputCode::KEY_F3;
                case VK_F4 : return proto::input::InputCode::KEY_F4;
                case VK_F5 : return proto::input::InputCode::KEY_F5;
                case VK_F6 : return proto::input::InputCode::KEY_F6;
                case VK_F7 : return proto::input::InputCode::KEY_F7;
                case VK_F8 : return proto::input::InputCode::KEY_F8;
                case VK_F9 : return proto::input::InputCode::KEY_F9;
                case VK_F10 : return proto::input::InputCode::KEY_F10;
                case VK_F11 : return proto::input::InputCode::KEY_F11;
                case VK_F12 : return proto::input::InputCode::KEY_F12;
                
                // Arrows
                case VK_UP : return proto::input::InputCode::KEY_UP;
                case VK_DOWN : return proto::input::InputCode::KEY_DOWN;
                case VK_LEFT : return proto::input::InputCode::KEY_LEFT;
                case VK_RIGHT : return proto::input::InputCode::KEY_RIGHT;
    
                // Modifiers - with L/R distinction
                case VK_SHIFT : return proto::input::InputCode::KEY_LSHIFT; // TODO
                //case 0x41 : return InputCode::KEY_RSHIFT; TODO
                case VK_CONTROL : return proto::input::InputCode::KEY_LCTRL;
                //case 0x41 : return InputCode::KEY_RCTRL; TODO
                case VK_MENU : return proto::input::InputCode::KEY_LALT; // TODO
                //case 0x41 : return InputCode::KEY_RALT; //TODO

                case VK_SPACE : return proto::input::InputCode::KEY_SPACE;
                case VK_TAB : return proto::input::InputCode::KEY_TAB;
                case VK_RETURN : return proto::input::InputCode::KEY_ENTER;
                case VK_ESCAPE : return proto::input::InputCode::KEY_ESCAPE;
                case VK_BACK : return proto::input::InputCode::KEY_BACKSPACE;
                
                // Special
                case VK_CAPITAL : return proto::input::InputCode::KEY_CAPSLOCK;
                case VK_INSERT : return proto::input::InputCode::KEY_INSERT;
                case VK_DELETE : return proto::input::InputCode::KEY_DELETE;
                case VK_HOME : return proto::input::InputCode::KEY_HOME;
                case VK_END : return proto::input::InputCode::KEY_END;
                case VK_PRIOR : return proto::input::InputCode::KEY_PAGEUP;
                case VK_NEXT : return proto::input::InputCode::KEY_PAGEDOWN;
                
                // Numpad
                case VK_NUMPAD0 : return proto::input::InputCode::KEY_NUMPAD_0;
                case VK_NUMPAD1 : return proto::input::InputCode::KEY_NUMPAD_1;
                case VK_NUMPAD2 : return proto::input::InputCode::KEY_NUMPAD_2;
                case VK_NUMPAD3 : return proto::input::InputCode::KEY_NUMPAD_3;
                case VK_NUMPAD4 : return proto::input::InputCode::KEY_NUMPAD_4;
                case VK_NUMPAD5 : return proto::input::InputCode::KEY_NUMPAD_5;
                case VK_NUMPAD6 : return proto::input::InputCode::KEY_NUMPAD_6;
                case VK_NUMPAD7 : return proto::input::InputCode::KEY_NUMPAD_7;
                case VK_NUMPAD8 : return proto::input::InputCode::KEY_NUMPAD_8;
                case VK_NUMPAD9 : return proto::input::InputCode::KEY_NUMPAD_9;
                case VK_ADD : return proto::input::InputCode::KEY_NUMPAD_ADD;
                case VK_SUBTRACT : return proto::input::InputCode::KEY_NUMPAD_SUB;
                case VK_MULTIPLY : return proto::input::InputCode::KEY_NUMPAD_MUL;
                case VK_DIVIDE : return proto::input::InputCode::KEY_NUMPAD_DIV;
                //case 0x41 : return InputCode::KEY_NUMPAD_ENTER; TODO
                
                // Mouse buttons
                case VK_LBUTTON : return proto::input::InputCode::MOUSE_LEFT;
                case VK_RBUTTON : return proto::input::InputCode::MOUSE_RIGHT;
                case VK_MBUTTON : return proto::input::InputCode::MOUSE_MIDDLE;
                case VK_XBUTTON1 : return proto::input::InputCode::MOUSE_X1;
                case VK_XBUTTON2 : return proto::input::InputCode::MOUSE_X2;


                default:
                    return proto::input::InputCode::UNKNOWN_InputCode;
            }

        return proto::input::InputCode::UNKNOWN_InputCode;
    }
}

namespace astre::input
{
    proto::input::InputCode keyToInputCode(int key)
    {
        return windows::_keyToInputCode(key);
    }
}