
#include "native/native.h"
#include "input/input.hpp"

namespace astre::input::windows
{
    static InputCode _keyToInputCode(int key)
    {
            switch (key)
            { 
                // Letters
                case 0x41 : return InputCode::KEY_A;
                case 0x42 : return InputCode::KEY_B;
                case 0x43 : return InputCode::KEY_C;
                case 0x44 : return InputCode::KEY_D;
                case 0x45 : return InputCode::KEY_E;
                case 0x46 : return InputCode::KEY_F;
                case 0x47 : return InputCode::KEY_G;
                case 0x48 : return InputCode::KEY_H;
                case 0x49 : return InputCode::KEY_I;
                case 0x4A : return InputCode::KEY_J;
                case 0x4B : return InputCode::KEY_K;
                case 0x4C : return InputCode::KEY_L;
                case 0x4D : return InputCode::KEY_M;
                case 0x4E : return InputCode::KEY_N;
                case 0x4F : return InputCode::KEY_O;
                case 0x50 : return InputCode::KEY_P;
                case 0x51 : return InputCode::KEY_Q;
                case 0x52 : return InputCode::KEY_R;
                case 0x53 : return InputCode::KEY_S;
                case 0x54 : return InputCode::KEY_T;
                case 0x55 : return InputCode::KEY_U;
                case 0x56 : return InputCode::KEY_V;
                case 0x57 : return InputCode::KEY_W;
                case 0x58 : return InputCode::KEY_X;
                case 0x59 : return InputCode::KEY_Y;
                case 0x5A : return InputCode::KEY_Z;

                // Numbers (Top row)
                case 0x30 : return InputCode::KEY_0;
                case 0x31 : return InputCode::KEY_1;
                case 0x32 : return InputCode::KEY_2;
                case 0x33 : return InputCode::KEY_3;
                case 0x34 : return InputCode::KEY_4;
                case 0x35 : return InputCode::KEY_5;
                case 0x36 : return InputCode::KEY_6;
                case 0x37 : return InputCode::KEY_7;
                case 0x38 : return InputCode::KEY_8;
                case 0x39 : return InputCode::KEY_9;
                
                // Function keys
                case VK_F1 : return InputCode::KEY_F1;
                case VK_F2 : return InputCode::KEY_F2;
                case VK_F3 : return InputCode::KEY_F3;
                case VK_F4 : return InputCode::KEY_F4;
                case VK_F5 : return InputCode::KEY_F5;
                case VK_F6 : return InputCode::KEY_F6;
                case VK_F7 : return InputCode::KEY_F7;
                case VK_F8 : return InputCode::KEY_F8;
                case VK_F9 : return InputCode::KEY_F9;
                case VK_F10 : return InputCode::KEY_F10;
                case VK_F11 : return InputCode::KEY_F11;
                case VK_F12 : return InputCode::KEY_F12;
                
                // Arrows
                case VK_UP : return InputCode::KEY_UP;
                case VK_DOWN : return InputCode::KEY_DOWN;
                case VK_LEFT : return InputCode::KEY_LEFT;
                case VK_RIGHT : return InputCode::KEY_RIGHT;
    
                // Modifiers - with L/R distinction
                case VK_SHIFT : return InputCode::KEY_LSHIFT; // TODO
                //case 0x41 : return InputCode::KEY_RSHIFT; TODO
                case VK_CONTROL : return InputCode::KEY_LCTRL;
                //case 0x41 : return InputCode::KEY_RCTRL; TODO
                case VK_MENU : return InputCode::KEY_LALT; // TODO
                //case 0x41 : return InputCode::KEY_RALT; //TODO

                case VK_SPACE : return InputCode::KEY_SPACE;
                case VK_TAB : return InputCode::KEY_TAB;
                case VK_RETURN : return InputCode::KEY_ENTER;
                case VK_ESCAPE : return InputCode::KEY_ESCAPE;
                case VK_BACK : return InputCode::KEY_BACKSPACE;
                
                // Special
                case VK_CAPITAL : return InputCode::KEY_CAPSLOCK;
                case VK_INSERT : return InputCode::KEY_INSERT;
                case VK_DELETE : return InputCode::KEY_DELETE;
                case VK_HOME : return InputCode::KEY_HOME;
                case VK_END : return InputCode::KEY_END;
                case VK_PRIOR : return InputCode::KEY_PAGEUP;
                case VK_NEXT : return InputCode::KEY_PAGEDOWN;
                
                // Numpad
                case VK_NUMPAD0 : return InputCode::KEY_NUMPAD_0;
                case VK_NUMPAD1 : return InputCode::KEY_NUMPAD_1;
                case VK_NUMPAD2 : return InputCode::KEY_NUMPAD_2;
                case VK_NUMPAD3 : return InputCode::KEY_NUMPAD_3;
                case VK_NUMPAD4 : return InputCode::KEY_NUMPAD_4;
                case VK_NUMPAD5 : return InputCode::KEY_NUMPAD_5;
                case VK_NUMPAD6 : return InputCode::KEY_NUMPAD_6;
                case VK_NUMPAD7 : return InputCode::KEY_NUMPAD_7;
                case VK_NUMPAD8 : return InputCode::KEY_NUMPAD_8;
                case VK_NUMPAD9 : return InputCode::KEY_NUMPAD_9;
                case VK_ADD : return InputCode::KEY_NUMPAD_ADD;
                case VK_SUBTRACT : return InputCode::KEY_NUMPAD_SUB;
                case VK_MULTIPLY : return InputCode::KEY_NUMPAD_MUL;
                case VK_DIVIDE : return InputCode::KEY_NUMPAD_DIV;
                //case 0x41 : return InputCode::KEY_NUMPAD_ENTER; TODO
                
                // Mouse buttons
                case VK_LBUTTON : return InputCode::MOUSE_LEFT;
                case VK_RBUTTON : return InputCode::MOUSE_RIGHT;
                case VK_MBUTTON : return InputCode::MOUSE_MIDDLE;
                case VK_XBUTTON1 : return InputCode::MOUSE_X1;
                case VK_XBUTTON2 : return InputCode::MOUSE_X2;


                default:
                    return InputCode::UNKNOWN_InputCode;
            }

        return InputCode::UNKNOWN_InputCode;
    }
}

namespace astre::input
{
    InputCode keyToInputCode(int key)
    {
        return windows::_keyToInputCode(key);
    }
}