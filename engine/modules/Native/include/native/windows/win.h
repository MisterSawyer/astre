#pragma once

#ifndef WIN32
    #error "Error, WIN32 not defined"
#endif

#ifndef _WIN32_WINNT
#define _WIN32_WINNT	0xa00
#endif
#ifdef UNICODE
  #undef UNICODE
#endif

#ifndef NOMINMAX
    #define NOMINMAX
#endif

#include <format>

#include <winsock2.h>
#include <windows.h>
#include <windowsx.h>

namespace astre::native
{
    using opengl_context_handle = HGLRC;
    using device_context = HDC;
    using console_handle = HANDLE;
    using window_handle = HWND;
    using process_handle = HANDLE;
    using procedure = LRESULT(CALLBACK*)(HWND, UINT, WPARAM, LPARAM);
}