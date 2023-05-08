#include "hello_triangle_application.h"

#include <chrono>

#include "log.h"

void HelloTriangleApplication::OnInit() {
    LOGD("OnInit");
}

void HelloTriangleApplication::OnRelease() {
    LOGD("OnRelease");

}

void HelloTriangleApplication::OnUpdate() {
    PrintFPS();

}

void HelloTriangleApplication::OnRender() {

}

void HelloTriangleApplication::PrintFPS() {
    static uint64_t frame_counter = 0;
    static double elapsed_seconds = 0.0;
    static std::chrono::high_resolution_clock clock{};
    static auto t0 = clock.now();

    frame_counter++;
    auto t1 = clock.now();
    auto delta_time = t1 - t0;
    t0 = t1;
    elapsed_seconds += delta_time.count() * 1e-9;
    if (elapsed_seconds > 1.0) {
        auto fps = frame_counter / elapsed_seconds;
        LOGD("FPS: %f", fps);
        frame_counter = 0;
        elapsed_seconds = 0.0;
    }
}