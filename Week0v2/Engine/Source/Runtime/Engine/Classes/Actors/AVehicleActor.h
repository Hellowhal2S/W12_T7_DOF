#pragma once
#include "GameFramework/Actor.h"
#include "GameFramework/Pawn.h"
#include "Physics/Car/SnippetVehicleCreate.h"

class UInputComponent;
struct VehicleInstance;
class VehicleManager;
class UStaticMeshComponent;

namespace physx { class PxVehicleDrive4W; }

class AVehicleActor : public APawn
{
    DECLARE_CLASS(AVehicleActor, APawn)
    
public:
    AVehicleActor();
    AVehicleActor(const AVehicleActor& Other);

    virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;
    virtual void Tick(float DeltaTime) override;

    snippetvehicle::VehicleDesc CreateDesc();
    void CreateVehicle();

    UStaticMeshComponent* GetStaticMeshComponent() const { return StaticMeshComponent; }

private:
    void MoveForward(float Value);
    void MoveRight  (float Value);
    void OnHandBrakePressed();
    void OnHandBrakeReleased();
    
    UStaticMeshComponent* StaticMeshComponent = nullptr;
    VehicleManager*      VehicleManager = nullptr;
    VehicleInstance*     VehicleInstance = nullptr;
    TArray<UStaticMeshComponent*> TireMeshes;
    
    float Throttle = 0.f, Brake = 0.f, Steering = 0.f;
    bool  bHandBrake = false;
};
