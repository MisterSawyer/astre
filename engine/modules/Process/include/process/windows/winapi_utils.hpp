#pragma once

#include "native/native.h"

namespace astre::process::windows
{
    template<class Extra, class ProcedureFunc>
    inline WNDCLASSEX defaultWindowClass(ProcedureFunc && proc, const std::string & class_name)
    {
        WNDCLASSEX wcex = {0};
        wcex.cbSize = sizeof(WNDCLASSEX);
        wcex.cbClsExtra = 0;
        wcex.hInstance = GetModuleHandle(nullptr);
        wcex.hCursor = LoadCursor(NULL, IDC_ARROW);
        wcex.hIconSm = LoadIcon(NULL, IDI_APPLICATION);
        wcex.hIcon = LoadIcon(NULL, IDI_APPLICATION);
        wcex.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
        wcex.lpfnWndProc = proc;
        wcex.cbWndExtra  = sizeof(Extra);
        wcex.lpszClassName = class_name.c_str();
        wcex.lpszMenuName = NULL;
        wcex.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC ;
        return wcex;
    }

    
    PIXELFORMATDESCRIPTOR generateAdvancedPFD();

    void lockCursorToCenter(HWND hwnd);
}