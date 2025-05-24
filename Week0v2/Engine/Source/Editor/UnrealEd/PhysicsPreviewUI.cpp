#include "PhysicsPreviewUI.h"
#include "EditorPanel.h"

#include "PropertyEditor/AnimSequenceEditorPanel.h"
#include "PropertyEditor/OutlinerEditorPanel.h"
#include "PropertyEditor/PreviewControlEditorPanel.h"
#include "PropertyEditor/PrimitiveDrawEditor.h"
#include "PropertyEditor/SkeletonPanel.h"
#include "ViewerPanel/PhysicsToolBoxPanel.h"
#include "ViewerPanel/ViewerControlPanel.h"
#include "ViewerPanel/ViewerMenuPanel.h"

void FPhysicsPreviewUI::Initialize(SLevelEditor* LevelEditor, float Width, float Height)
{
    auto MenuPanel = std::make_shared<ViewerMenuPanel>();
    Panels["MenuPanel"] = MenuPanel;
    
    auto ControlPanel = std::make_shared<ViewerControlPanel>();
    // ControlPanel->Initialize(LevelEditor, Width, Height);
    Panels["PreviewControlPanel"] = ControlPanel;
    
    auto PropertyPanel = std::make_shared<FSkeletonPanel>();
    PropertyPanel->Initialize(Width, Height);   
    Panels["PropertyPanel"] = PropertyPanel;
    
    auto PrimitiveDrawer = std::make_shared<PrimitiveDrawEditor>();
    Panels["PrimitiveDrawEditor"] = PrimitiveDrawer;

    auto PhysicsToolboxPanel = std::make_shared<FPhysicsToolBoxPanel>();
    Panels["ToolBox"] = PhysicsToolboxPanel;
}

void FPhysicsPreviewUI::Render() const
{
    for (const auto& Panel : Panels)
    {
        if(Panel.Value->bIsVisible)
        {
            Panel.Value->Render();
        }
    }
}

void FPhysicsPreviewUI::AddEditorPanel(const FString& PanelId, const std::shared_ptr<UEditorPanel>& EditorPanel)
{
    Panels[PanelId] = EditorPanel;
}

void FPhysicsPreviewUI::OnResize(HWND hWnd) const
{
    for (auto& Panel : Panels)
    {
        Panel.Value->OnResize(hWnd);
    }
}

void FPhysicsPreviewUI::SetWorld(UWorld* InWorld)
{
    World = InWorld;
    for (auto& [_, Panel] : Panels)
    {
        Panel->SetWorld(World);
    } 
}

std::shared_ptr<UEditorPanel> FPhysicsPreviewUI::GetEditorPanel(const FString& PanelId)
{
    return Panels[PanelId];
}
