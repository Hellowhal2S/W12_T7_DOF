#include "VehicleManager.h"
#include "Physics/PhysX.h"
#include <vehicle/PxVehicleUtilControl.h>
#include "SnippetVehicleSceneQuery.h"
#include "Actors/AVehicleActor.h"

static PxF32 gSteerVsForwardSpeedData[2*8]=
{
    0.0f,		0.75f,
    5.0f,		0.75f,
    30.0f,		0.125f,
    120.0f,		0.1f,
    PX_MAX_F32, PX_MAX_F32,
    PX_MAX_F32, PX_MAX_F32,
    PX_MAX_F32, PX_MAX_F32,
    PX_MAX_F32, PX_MAX_F32
};
PxFixedSizeLookupTable<8> gSteerVsForwardSpeedTable(gSteerVsForwardSpeedData,4);

static PxVehicleKeySmoothingData gKeySmoothingDataDefault=
{
    {
        6.0f,	//rise rate eANALOG_INPUT_ACCEL
        6.0f,	//rise rate eANALOG_INPUT_BRAKE		
        6.0f,	//rise rate eANALOG_INPUT_HANDBRAKE	
        2.5f,	//rise rate eANALOG_INPUT_STEER_LEFT
        2.5f,	//rise rate eANALOG_INPUT_STEER_RIGHT
    },
    {
        10.0f,	//fall rate eANALOG_INPUT_ACCEL
        10.0f,	//fall rate eANALOG_INPUT_BRAKE		
        10.0f,	//fall rate eANALOG_INPUT_HANDBRAKE	
        5.0f,	//fall rate eANALOG_INPUT_STEER_LEFT
        5.0f	//fall rate eANALOG_INPUT_STEER_RIGHT
    }
};

VehicleManager::VehicleManager(physx::PxScene* scene)
    : PxScene(scene), PxPhysics(gPhysics), DefaultMaterial(gMaterial), PxCooking(gCooking),
      SteerVsForwardSpeedTable(gSteerVsForwardSpeedData, 4),
      KeySmoothingData(gKeySmoothingDataDefault),
      MimicKeyInputs(true)
{
    if (!PxScene || !PxPhysics || !DefaultMaterial)
    {
        printf("VehicleManager: Invalid PxScene, PxPhysics, or PxMaterial provided!\n");
        return;
    }
}

VehicleManager::~VehicleManager()
{
    if (PlayerVehicleInstance.vehicle)
    {
        PlayerVehicleInstance.vehicle->getRigidDynamicActor()->release(); // 섀시 액터 해제
        PlayerVehicleInstance.vehicle->free();                            // PxVehicleDrive4W 객체 해제
        PlayerVehicleInstance.vehicle = nullptr;
        PlayerVehicleInstance.chassisActor = nullptr;
        PlayerVehicleInstance.releaseWheelQueryResults(gAllocator); // 전역 gAllocator 사용
    }
}

void VehicleManager::UpdateAllVehicles(float dt)
{
    for (auto Actor : VehicleActors)
        Actor->Tick(dt);
    
    if (!PlayerVehicleInstance.vehicle || !PxScene || !gFrictionPairs) return;

    // 1. 제어 입력 업데이트 (부드럽게 적용)
    // if (MimicKeyInputs)
    // {
    //     PxVehicleDrive4WSmoothDigitalRawInputsAndSetAnalogInputs(
    //         KeySmoothingData, SteerVsForwardSpeedTable,
    //         PlayerVehicleInstance.inputData, dt, PlayerVehicleInstance.isInAir,
    //         *PlayerVehicleInstance.vehicle);
    // }
    // else
    // {
    //     PxVehicleDrive4WSmoothAnalogRawInputsAndSetAnalogInputs(
    //         PxVehiclePadSmoothingData(), // 기본 또는 커스텀 PadSmoothingData
    //         SteerVsForwardSpeedTable,
    //         PlayerVehicleInstance.inputData, dt, PlayerVehicleInstance.isInAir,
    //         *PlayerVehicleInstance.vehicle);
    // }

    // 2. 서스펜션 레이캐스트
    // PxVehicleWheels* vehicles[1] = {PlayerVehicleInstance.vehicle};
    // // PxVehicleSuspensionRaycasts 함수는 PxRaycastQueryResult 배열을 직접 받음
    // PxVehicleSuspensionRaycasts(BatchQuery, 1, vehicles,
    //                             SuspensionRaycastData.nbSqResults, 
    //                             SuspensionRaycastData.sqResults);

    // 3. 차량 물리 업데이트
    // const PxVec3 grav = PxScene->getGravity();
    // PxVehicleUpdates(dt, grav, *gFrictionPairs, 1, vehicles, &PlayerVehicleInstance.wheelQueryResults);
    //
    // // 4. 차량 공중 상태 업데이트
    // PlayerVehicleInstance.isInAir = PlayerVehicleInstance.vehicle->getRigidDynamicActor()->isSleeping() ?
    //                                 false : PxVehicleIsInAir(PlayerVehicleInstance.wheelQueryResults);
}

void VehicleManager::UpdatePlayerVehicleInput(float accel, float brake, float steer, bool handbrake)
{
    if (!PlayerVehicleInstance.vehicle) return;

    if (MimicKeyInputs)
    {
        PlayerVehicleInstance.inputData.setDigitalAccel(accel > 0.1f);
        PlayerVehicleInstance.inputData.setDigitalBrake(brake > 0.1f);
        PlayerVehicleInstance.inputData.setDigitalSteerLeft(steer < -0.1f);
        PlayerVehicleInstance.inputData.setDigitalSteerRight(steer > 0.1f);
        PlayerVehicleInstance.inputData.setDigitalHandbrake(handbrake);
    }
    else
    {
        PlayerVehicleInstance.inputData.setAnalogAccel(accel);
        PlayerVehicleInstance.inputData.setAnalogBrake(brake);
        PlayerVehicleInstance.inputData.setAnalogSteer(steer);
        PlayerVehicleInstance.inputData.setAnalogHandbrake(handbrake ? 1.0f : 0.0f);
    }
}