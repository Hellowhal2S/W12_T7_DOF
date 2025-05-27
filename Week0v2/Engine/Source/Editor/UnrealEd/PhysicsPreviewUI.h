#pragma once
#include "Container/Map.h"
#include "Container/String.h"
#include "Engine/World.h"

class UEditorPanel;
class SLevelEditor;
class FPhysicsPreviewUI
{
public:
    FPhysicsPreviewUI() = default;
    ~FPhysicsPreviewUI() = default;
    void Initialize(SLevelEditor* LevelEditor, float Width, float Height);
    
    void Render() const;
    void OnResize(HWND hWnd) const;
    void SetWorld(UWorld* InWorld);
    
    void AddEditorPanel(const FString& PanelId, const std::shared_ptr<UEditorPanel>& EditorPanel);
    std::shared_ptr<UEditorPanel> GetEditorPanel(const FString& PanelId);

    void SetSkeletalMesh(USkeletalMesh* InSkeletalMesh);
private:
    TMap<FString, std::shared_ptr<UEditorPanel>> Panels;
    UWorld* World = nullptr;
};
