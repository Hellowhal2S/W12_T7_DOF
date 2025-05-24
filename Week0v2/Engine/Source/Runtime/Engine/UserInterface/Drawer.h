#pragma once
#include "UnrealEd/EditorPanel.h"

class FDrawer : public UEditorPanel, IWindowToggleable
{
public:
    FDrawer() = default;
    ~FDrawer() override = default;

    // No copy constructor
    FDrawer(const FDrawer&) = delete;
    FDrawer& operator=(const FDrawer&) = delete;
    FDrawer(FDrawer&&) = delete;
    FDrawer& operator=(FDrawer&&) = delete;

public:
    /** Override */
    void Toggle() override;
    
public:
    virtual void Render();
    void Render(float DeltaTime);
    void OnResize(HWND hWnd) override;

    void RenderContentDrawer();
private:
    bool bIsOpen = false;

    float AnimationTime = 0.0f;
    const float AnimationDuration = 0.25f; // 초 단위
    bool bFirstOpenFrame = false;
    
    float Width = 0.0f, Height = 0.0f;
};
