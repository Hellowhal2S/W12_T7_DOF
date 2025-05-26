#pragma once
#include "Actors/ADodge.h"


class UBodySetup;

// TODO : 시간이 되면 스켈레톤 조종을 여기다 구현할 것 
class FSkeletonDetailPanel
{
public:
    void Render(UBodySetup* BodySetup);
    void OnResize(HWND hWnd);
private:
    float Width = 800, Height = 600;
};
