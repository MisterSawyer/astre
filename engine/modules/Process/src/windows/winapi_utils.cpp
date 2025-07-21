#include "winapi_utils.hpp"

namespace astre::process::windows
{
    PIXELFORMATDESCRIPTOR generateAdvancedPFD()
    {
	    PIXELFORMATDESCRIPTOR pfd = {0};
	    pfd.nSize = sizeof(PIXELFORMATDESCRIPTOR);
	    pfd.nVersion = 1;
	    pfd.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
	    pfd.cColorBits = 24;
	    pfd.cAlphaBits = 8;
	    pfd.cDepthBits = 24;
	    pfd.cStencilBits = 8;
	    pfd.iPixelType = PFD_TYPE_RGBA;
	    pfd.iLayerType = PFD_MAIN_PLANE;
	    return pfd;
    }

    void lockCursorToCenter(HWND hwnd) 
    {
        RECT client_rect;
        GetClientRect(hwnd, &client_rect);

        POINT center = {
            (client_rect.right - client_rect.left) / 2,
            (client_rect.bottom - client_rect.top) / 2
        };

        // Convert to screen coordinates
        ClientToScreen(hwnd, &center);
        SetCursorPos(center.x, center.y);

        // Confine the cursor to the window
        RECT clip_rect;
        GetWindowRect(hwnd, &clip_rect);
        ClipCursor(&clip_rect);

        ShowCursor(FALSE); // Hide the cursor
    }
}