#pragma once
#include "UnrealEd/EditorPanel.h"

class FPhysicsToolBoxPanel : public UEditorPanel
{
public:
public:
    void Render();
    void OnResize(HWND hWnd);

private:
    float Width = 800, Height = 600;
};
