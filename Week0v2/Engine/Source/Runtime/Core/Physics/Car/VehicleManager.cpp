#include "VehicleManager.h"
#include "Physics/PhysX.h"
#include <vehicle/PxVehicleUtilControl.h>
#include <vehicle/PxVehicleUtilSetup.h>

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
    : PxScene(scene), PxPhysics(gPhysics), DefaultMaterial(gMaterial),
      SteerVsForwardSpeedTable(gSteerVsForwardSpeedData, 4),
      KeySmoothingData(gKeySmoothingDataDefault),
      MimicKeyInputs(true)
{
    if (!PxScene || !PxPhysics || !DefaultMaterial)
    {
        printf("VehicleManager: Invalid PxScene, PxPhysics, or PxMaterial provided!\n");
        return;
    }

    if (!SetupSuspensionRaycast())
    {
        printf("VehicleManager: Failed to setup suspension raycast!\n");
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

    ReleaseVehicleSetupData(); // WheelsSimData, DriveSimData4W 해제
    ReleaseSuspensionRaycast();
}

bool VehicleManager::InitVehicleSetupData()
{
    WheelsSimData = PxVehicleWheelsSimData::allocate(NumWheelsPerVehicle);
    if (!WheelsSimData)
        return false;

    return true;
}

void VehicleManager::ReleaseVehicleSetupData()
{
    if (WheelsSimData)
    {
        WheelsSimData->free();
        WheelsSimData = nullptr;
    }
}

static PxVehicleChassisData createChassisData()
{
    PxVehicleChassisData chassisData;
    chassisData.mMass = 1500.0f;
    chassisData.mMOI = PxVec3((2.0f*2.0f + 5.0f*5.0f)*1500.0f/12.0f,
                                (2.5f*2.5f + 5.0f*5.0f)*0.8f*1500.0f/12.0f,
                                (2.5f*2.5f + 2.0f*2.0f)*1500.0f/12.0f);
    chassisData.mCMOffset = PxVec3(0.0f, -2.5f*0.5f + 0.65f, 0.25f); // Y-up 기준, Z-up으로 변환 필요
    chassisData.mCMOffset = PxVec3(0.25f, 0.0f, 0.65f);
    return chassisData;
}

// 예시: 휠/타이어/서스펜션 데이터 설정 (실제로는 SnippetVehicleCreate.cpp의 createWheelsSimData 참고)
static void setupDefaultWheelsSimData(physx::PxVehicleWheelsSimData& wheelsSimData, physx::PxU32 numWheels)
{
    // wheelsSimData.allocate(numWheels);
    // 각 휠, 타이어, 서스펜션 데이터 설정 (Snippet 코드 참고)
    // ...
    // 예: 앞바퀴 FL, FR, 뒷바퀴 RL, RR
    // Z-up, X-Forward 기준 오프셋 설정
    // PxVec3 wheelCenterActorOffsets[4] = { PxVec3(1.5f, 0.8f, -0.3f), PxVec3(1.5f, -0.8f, -0.3f), ... };
    // for (PxU32 i=0; i<numWheels; ++i) wheelsSimData.setWheelCentreOffset(i, wheelCenterActorOffsets[i]);
}

// 예시: 구동계 데이터 설정 (실제로는 SnippetVehicleCreate.cpp의 createDriveSimData4W 참고)
static void setupDefaultDriveSimData4W(PxVehicleDriveSimData4W& driveSimData) {
    // 엔진, 기어, 클러치, 디퍼렌셜 데이터 설정 (Snippet 코드 참고)
    // ...
}

bool VehicleManager::CreatePlayerVehicle(const PxTransform& initialPose)
{
    if (!WheelsSimData || !PxPhysics || !DefaultMaterial || !PxScene)
        return false;
    
    // 1. 차량 데이터 설정 (WheelsSimData, DriveSimData4W 채우기)
    // SnippetVehicleCreate.cpp의 createWheelsSimData, createDriveSimData4W 로직 참고
    setupDefaultWheelsSimData(*WheelsSimData, NumWheelsPerVehicle);
    setupDefaultDriveSimData4W(DriveSimData4W);                    

    // 2. 섀시 데이터 준비
    physx::PxVehicleChassisData chassisData = createChassisData();

    // 3. 섀시 액터 생성 (SnippetVehicleCreate.cpp의 createChassis 참고)
    // 주의: Snippet은 Y-up 기준. Z-up으로 변환 필요.
    // 예: chassisDims(2.5f,2.0f,5.0f) -> PxBoxGeometry(5.0f*0.5f, 2.5f*0.5f, 2.0f*0.5f) (X,Y,Z 순서 가정)
    physx::PxBoxGeometry chassisGeom(2.5f, 1.0f, 0.5f); // Z-up, X-Forward 기준 섀시 half-extents (길이, 폭, 높이)
    
    PlayerVehicleInstance.chassisActor = PxPhysics->createRigidDynamic(initialPose);
    physx::PxShape* chassisShape = physx::PxRigidActorExt::createExclusiveShape(*PlayerVehicleInstance.chassisActor, chassisGeom, *DefaultMaterial);
    
    // 섀시 필터 데이터 설정 (프로젝트의 FPhysX::MakeFilterData 사용)
    chassisShape->setSimulationFilterData(FPhysX::MakeFilterData(FPhysX::ECollisionGroup::Player, FPhysX::ECollisionGroup::All));
    // 쿼리 필터 데이터도 필요시 설정 (레이캐스트가 섀시와 충돌하지 않도록)
    // PxFilterData chassisQueryFilterData(0,0,0,DISABLE_VEHICLE_RAYCAST_AGAINST_CHASSIS_FLAG); // 가상 플래그
    // chassisShape->setQueryFilterData(chassisQueryFilterData);

    PlayerVehicleInstance.chassisActor->setMass(chassisData.mMass);
    PlayerVehicleInstance.chassisActor->setMassSpaceInertiaTensor(chassisData.mMOI);
    PlayerVehicleInstance.chassisActor->setCMassLocalPose(physx::PxTransform(chassisData.mCMOffset, physx::PxQuat(physx::PxIdentity)));
    
    PxScene->addActor(*PlayerVehicleInstance.chassisActor);
    PlayerVehicleInstance.chassisActor->setName("PlayerVehicleChassis");

    // 4. PxVehicleDrive4W 객체 생성
    PlayerVehicleInstance.vehicle = physx::PxVehicleDrive4W::allocate(NumWheelsPerVehicle);
    if (!PlayerVehicleInstance.vehicle) {
        PlayerVehicleInstance.chassisActor->release();
        PlayerVehicleInstance.chassisActor = nullptr;
        return false;
    }

    PlayerVehicleInstance.vehicle->setup(PxPhysics, PlayerVehicleInstance.chassisActor, *WheelsSimData, DriveSimData4W, 0);
    
    // 5. 차량 초기 상태 설정 (Snippet 참고)
    PlayerVehicleInstance.vehicle->setToRestState();
    PlayerVehicleInstance.vehicle->mDriveDynData.forceGearChange(physx::PxVehicleGearsData::eFIRST);
    PlayerVehicleInstance.vehicle->mDriveDynData.setUseAutoGears(true); // 또는 false

    // 6. 휠 쿼리 결과 버퍼 설정
    PlayerVehicleInstance.setupWheelQueryResults(NumWheelsPerVehicle, gAllocator);

    return true;
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

void VehicleManager::UpdateAllVehicles(float dt)
{
    if (!PlayerVehicleInstance.vehicle || !PxScene || !BatchQuery || !gFrictionPairs) return;

    // 1. 제어 입력 업데이트 (부드럽게 적용)
    if (MimicKeyInputs)
    {
        PxVehicleDrive4WSmoothDigitalRawInputsAndSetAnalogInputs(
            KeySmoothingData, SteerVsForwardSpeedTable,
            PlayerVehicleInstance.inputData, dt, PlayerVehicleInstance.isInAir,
            *PlayerVehicleInstance.vehicle);
    }
    else
    {
        PxVehicleDrive4WSmoothAnalogRawInputsAndSetAnalogInputs(
            PxVehiclePadSmoothingData(), // 기본 또는 커스텀 PadSmoothingData
            SteerVsForwardSpeedTable,
            PlayerVehicleInstance.inputData, dt, PlayerVehicleInstance.isInAir,
            *PlayerVehicleInstance.vehicle);
    }

    // 2. 서스펜션 레이캐스트
    PxVehicleWheels* vehicles[1] = {PlayerVehicleInstance.vehicle};
    // PxVehicleSuspensionRaycasts 함수는 PxRaycastQueryResult 배열을 직접 받음
    PxVehicleSuspensionRaycasts(BatchQuery, 1, vehicles,
                                SuspensionRaycastData.nbSqResults, 
                                SuspensionRaycastData.sqResults);
    // 중요: PxVehicleSuspensionRaycasts는 PxRaycastQueryResult를 채워주고,
    // PxVehicleUpdates는 PxVehicleWheelQueryResult를 필요로 합니다.
    // Snippet에서는 PxVehicleUpdates 호출 시 PxWheelQueryResult를 직접 생성해서 전달합니다.
    // PxVehicleSuspensionRaycasts의 결과(SuspensionRaycastData.sqResults)를
    // PlayerVehicleInstance.wheelQueryResults.wheelQueryResults에 복사/변환하는 과정이
    // PxVehicleUpdates 내부에서 일어나거나, 또는 사용자가 직접 해줘야 할 수 있습니다.
    // Snippet에서는 PxVehicleUpdates에 새로 할당한 PxWheelQueryResult 배열을 넘기고,
    // PxVehicleUpdates가 이 배열을 채웁니다. PxVehicleSuspensionRaycasts의 결과는
    // PxVehicleUpdates가 내부적으로 사용합니다.

    // 3. 차량 물리 업데이트
    const PxVec3 grav = PxScene->getGravity();
    // PxVehicleUpdates는 PxVehicleWheelQueryResult 배열의 포인터를 받습니다.
    PxVehicleUpdates(dt, grav, *gFrictionPairs, 1, vehicles, &PlayerVehicleInstance.wheelQueryResults);


    // 4. 차량 공중 상태 업데이트
    PlayerVehicleInstance.isInAir = PlayerVehicleInstance.vehicle->getRigidDynamicActor()->isSleeping() ?
                                    false : PxVehicleIsInAir(PlayerVehicleInstance.wheelQueryResults);
}

bool VehicleManager::SetupSuspensionRaycast()
{
    // 최대 휠 개수만큼 레이캐스트 결과 버퍼 할당 (플레이어 차량 1대 가정)
    SuspensionRaycastData.allocate(NumWheelsPerVehicle, gAllocator); // 전역 gAllocator 사용
    
    // BatchQuery 생성
    // Snippet에서는 VehicleSceneQueryData::setUpBatchedSceneQuery 에서 필터 콜백 등을 설정함.
    // 여기서는 간단히 PxBatchQueryDesc만 사용. 필요시 pre/post filter 콜백 설정.
    PxBatchQueryDesc sqDesc(NumWheelsPerVehicle, 0, 0);
    // sqDesc.queryMemory.userRaycastResultBuffer = SuspensionRaycastData.sqResults;
    // sqDesc.queryMemory.userRaycastTouchBuffer = SuspensionRaycastData.sqHitBuffer;
    // sqDesc.queryMemory.raycastTouchBufferSize = SuspensionRaycastData.nbSqResults;
    // sqDesc.preFilterShader = YourPreFilterShader; // 필요시 설정 (Snippet의 WheelSceneQueryPreFilterBlocking)

    BatchQuery = PxScene->createBatchQuery(sqDesc);
    if (!BatchQuery)
    {
        SuspensionRaycastData.free(gAllocator);
        return false;
    }
    return true;
}

void VehicleManager::ReleaseSuspensionRaycast()
{
    if (BatchQuery)
    {
        BatchQuery->release();
        BatchQuery = nullptr;
    }
    SuspensionRaycastData.free(gAllocator);
}
