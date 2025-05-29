#include "VehicleManager.h"
#include "Physics/PhysX.h"
#include <vehicle/PxVehicleUtilControl.h>
#include "SnippetVehicleSceneQuery.h"
#include "Actors/AVehicleActor.h"
#include "Physics/Car/TransformConvert.h"

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

    // 1) 씬 쿼리 데이터 할당
    SQData = snippetvehicle::VehicleSceneQueryData::allocate(
        /*maxNumVehicles=*/       MaxVehicles,           // 예: 1
        /*maxNumWheelsPerVehicle=*/NumWheelsPerVehicle, // 4
        /*maxNumHitPointsPerWheel=*/1,                   // 가까운 히트 한 개만
        /*numVehiclesInBatch=*/    1,                    // 한 대씩 배치
        snippetvehicle::WheelSceneQueryPreFilterBlocking,
        snippetvehicle::WheelSceneQueryPostFilterBlocking,
        gAllocator);

    // 2) PxBatchQuery 생성
    SQBatchQuery = snippetvehicle::VehicleSceneQueryData::setUpBatchedSceneQuery(
        /*batchId=*/0, *SQData, PxScene);

    // 3) 바퀴 수 만큼 결과 버퍼 할당
    SQResults.allocate(NumWheelsPerVehicle, gAllocator);
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

    if (SQBatchQuery)
        SQBatchQuery->release();
    if (SQData)
        SQData->free(gAllocator);
    SQResults.free(gAllocator);
}

void VehicleManager::AddVehicleActor(AVehicleActor* actor)
{
    VehicleActors.Add(actor);
}

void VehicleManager::RegisterPlayerVehicle(PxRigidDynamic* ChassisActor,
                                           PxVehicleDrive4W* Vehicle,
                                           PxU32             NumWheels)
{
    PlayerVehicleInstance.chassisActor = ChassisActor;
    PlayerVehicleInstance.vehicle      = Vehicle;
    PlayerVehicleInstance.setupWheelQueryResults(NumWheels, gAllocator);
}

void VehicleManager::UpdateAllVehicles(float dt)
{
    if (!PlayerVehicleInstance.vehicle) return;

    UE_LOG(LogLevel::Display, "Engine RPM: %f, Current Gear: %d", 
           PlayerVehicleInstance.vehicle->mDriveDynData.getEngineRotationSpeed(),
           PlayerVehicleInstance.vehicle->mDriveDynData.getCurrentGear());
    
    // 바퀴 접촉 상태 확인
    for (PxU32 i = 0; i < PlayerVehicleInstance.vehicle->mWheelsSimData.getNbWheels(); i++)
    {
        bool inContact = PlayerVehicleInstance.wheelQueryResults.wheelQueryResults[i].isInAir;
        UE_LOG(LogLevel::Display, "Wheel %d in air: %s", i, inContact ? "true" : "false");
    }
    
    // ① 입력 스무딩 & 아날로그 입력 세팅
    PxVehicleDrive4WSmoothAnalogRawInputsAndSetAnalogInputs(
        PxVehiclePadSmoothingData(),
        SteerVsForwardSpeedTable,
        PlayerVehicleInstance.inputData,
        dt,
        PlayerVehicleInstance.isInAir,
        *PlayerVehicleInstance.vehicle
    );

    // ② 서스펜션 레이캐스트
    PxVehicleWheels* vehicles[1] = { PlayerVehicleInstance.vehicle };
    PxVehicleSuspensionRaycasts(
    SQBatchQuery, 1, vehicles,
    SQResults.nbSqResults, SQResults.sqResults);
    
    // ③ 차량 물리 업데이트
    PxVehicleUpdates(
        dt,
        GetPxScene()->getGravity(),
        *gFrictionPairs,
        1,
        vehicles,
        &PlayerVehicleInstance.wheelQueryResults
    );

    // ④ 공중 상태 갱신
    PlayerVehicleInstance.isInAir =
      PxVehicleIsInAir(PlayerVehicleInstance.wheelQueryResults);

    for (auto Actor : VehicleActors)
    {
        PxTransform chassisPose = GetPlayerVehicle()->chassisActor->getGlobalPose();
        Actor->SetActorLocation(P2UVector(chassisPose.p));
        Actor->SetActorRotation(FRotator(P2UQuat  (chassisPose.q)));
    }
}

void VehicleManager::UpdatePlayerVehicleInput(float accel, float brake, float steer, bool handbrake)
{
    if (!PlayerVehicleInstance.vehicle) return;

    UE_LOG(LogLevel::Display, "UpdatePlayerVehicleInput : %f, %f, %f, %d", accel, brake, steer, handbrake);

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