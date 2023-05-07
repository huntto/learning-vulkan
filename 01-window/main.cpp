#include <windows.h>

#include <vulkan/vulkan.h>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/vec4.hpp>
#include <glm/mat4x4.hpp>

#include <malloc.h>
#include <tchar.h>


void TRACE(LPCTSTR lpszFmt, ...) {
    va_list args;
    va_start(args, lpszFmt);
    int len = _vsctprintf(lpszFmt, args) + 1;
    TCHAR* lpszBuf = (TCHAR*)_malloca(len * sizeof(TCHAR));
    if (lpszBuf != NULL) {
        _vstprintf_s(lpszBuf, len, lpszFmt, args);
        OutputDebugString(lpszBuf);
    }
    va_end(args);
    _freea(lpszBuf);
}

LRESULT CALLBACK WindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    }

    return DefWindowProc(hwnd, msg, wParam, lParam);
}

int WINAPI WinMain(_In_ HINSTANCE hInstance,
    _In_opt_ HINSTANCE hPrevInstance,
    _In_ LPSTR lpCmdLine,
    _In_ int nShowCmd) {
    LPCWSTR window_class = L"Learning Vulkan Class";
    LPCWSTR window_title = L"Learning Vulkan";
    int window_width = 800;
    int window_height = 600;

    WNDCLASSEX wcx{};
    wcx.cbSize = sizeof(wcx);
    wcx.style = CS_HREDRAW | CS_VREDRAW;
    wcx.lpfnWndProc = WindowProc;
    wcx.cbClsExtra = 0;
    wcx.cbWndExtra = 0;
    wcx.hInstance = hInstance;
    wcx.hIcon = LoadIcon(NULL, IDI_APPLICATION);
    wcx.hCursor = LoadCursor(NULL, IDC_ARROW);
    wcx.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wcx.lpszMenuName = NULL;
    wcx.lpszClassName = window_class;
    wcx.hIconSm = LoadIcon(hInstance, IDI_APPLICATION);;

    if (!RegisterClassEx(&wcx)) {
        MessageBox(NULL,
            L"Call to RegisterClassEx failed!",
            L"Learning Vulkan",
            NULL);
        return 1;
    }

    HWND hwnd = CreateWindowEx(
        WS_EX_OVERLAPPEDWINDOW,
        window_class,
        window_title,
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT,
        window_width, window_height,
        NULL,
        NULL,
        hInstance,
        NULL
    );

    if (!hwnd) {
        MessageBox(NULL,
            L"Call to CreateWindow failed!",
            L"Learning Vulkan",
            NULL);
        return 1;
    }

    ShowWindow(hwnd, nShowCmd);
    UpdateWindow(hwnd);

    uint32_t extensionCount = 0;
    vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);

    TRACE(L"%d extensions supported\n", extensionCount);

    glm::mat4 matrix;
    glm::vec4 vec;
    auto test = matrix * vec;

    MSG msg{};
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return (int)msg.wParam;
}
