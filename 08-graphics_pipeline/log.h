#pragma once

#ifdef _WIN32

#include <windows.h>
#include <malloc.h>
#include <tchar.h>
#include <stdio.h>

inline void LOGD(LPCTSTR fmt, ...) {
    va_list args;
    va_start(args, fmt);
    int len = _vsctprintf(fmt, args) + 1;
    TCHAR* buf = (TCHAR*)_malloca(len * sizeof(TCHAR));
    if (buf != NULL) {
        _vstprintf_s(buf, len, fmt, args);
        OutputDebugString(buf);
        OutputDebugString(L"\n");
        _freea(buf);
    }
    va_end(args);
}

inline void LOGD(LPCSTR fmt, ...) {
    va_list args;
    va_start(args, fmt);
    int len = _vscprintf(fmt, args) + 1;
    char* buf = (char*)_malloca(len * sizeof(char));
    if (buf != NULL) {
        vsprintf_s(buf, len, fmt, args);
        OutputDebugStringA(buf);
        OutputDebugStringA("\n");
        _freea(buf);
    }
    va_end(args);
}

#endif