#pragma once

class HelloTriangleApplication {
public:
    virtual void OnInit();
    virtual void OnRelease();
    virtual void OnRender();
    virtual void OnUpdate();
protected:
    void PrintFPS();
};