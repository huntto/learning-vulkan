#include <windows.h>

#include "hello_triangle_application.h"

#include "log.h"

LRESULT CALLBACK WindowProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) {
    HelloTriangleApplication* app = reinterpret_cast<HelloTriangleApplication*>(GetWindowLongPtr(hwnd, GWLP_USERDATA));

    switch (message) {
    case WM_CREATE:
    {
        LPCREATESTRUCT pCreateStruct = reinterpret_cast<LPCREATESTRUCT>(lParam);
        SetWindowLongPtr(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(pCreateStruct->lpCreateParams));
        HelloTriangleApplication* app = reinterpret_cast<HelloTriangleApplication*>(pCreateStruct->lpCreateParams);
        HINSTANCE hinstance = GetModuleHandle(0);
        if (app) {
            app->OnWindowCreated(hinstance, hwnd);
        }
    }
    return 0;
    case WM_SIZE:
        if (app) {
            app->OnWindowSizeChanged();
        }
    case WM_PAINT:
    case WM_ERASEBKGND:
        if (app) {
            app->OnUpdate();
            app->OnRender();
        }
        return 0;
    case WM_DESTROY:
        if (app) {
            app->OnWindowDestroyed();
        }
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProc(hwnd, message, wParam, lParam);
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

    HelloTriangleApplication app;

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
        &app
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

    MSG msg = {};
    while (msg.message != WM_QUIT) {
        if (::PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
            ::TranslateMessage(&msg);
            ::DispatchMessage(&msg);
        }
    }

    return (int)msg.wParam;
}
