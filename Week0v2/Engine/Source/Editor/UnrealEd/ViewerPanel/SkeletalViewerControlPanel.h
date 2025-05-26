#pragma once
#include "UnrealEd/EditorPanel.h"

class FSkeletalViewerControlPanel: public UEditorPanel
{
public:
    virtual void Render() override;
    virtual void OnResize(HWND hWnd) override;

    void SetSkeletalMesh(USkeletalMesh* mesh) override
    {
        SkeletalMesh = mesh;
    }
private:
    float Width = 300, Height = 100;
    USkeletalMesh* SkeletalMesh;
};
