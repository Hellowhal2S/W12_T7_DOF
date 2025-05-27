#include "VehicleManager.h"
#include "Physics/PhysX.h"
#include <vehicle/PxVehicleUtilControl.h>
#include <vehicle/PxVehicleUtilSetup.h>
#include "SnippetVehicleSceneQuery.h"

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
    
    // Z-up, X-Forward 기준 섀시 크기 (길이X, 폭Y, 높이Z)
    const PxVec3 chassisDims(4.5f, 2.0f, 1.4f);
    
    chassisData.mMOI = PxVec3(
        (chassisDims.y*chassisDims.y + chassisDims.z*chassisDims.z)*chassisData.mMass/12.0f, // X축 회전 관성
        (chassisDims.x*chassisDims.x + chassisDims.z*chassisDims.z)*chassisData.mMass/12.0f, // Y축 회전 관성
        (chassisDims.x*chassisDims.x + chassisDims.y*chassisDims.y)*chassisData.mMass/12.0f  // Z축 회전 관성
    );
    // Z-up, X-Forward 기준 무게 중심 오프셋 (X:앞뒤, Y:좌우, Z:상하) - 차량 중심 기준
    chassisData.mCMOffset = PxVec3(0.1f, 0.0f, 0.3f); // 예: 약간 앞쪽, 약간 위쪽
    return chassisData;
}

// 휠/타이어/서스펜션 데이터 설정
static void setupDefaultWheelsSimData(PxVehicleWheelsSimData& wheelsSimData, PxU32 numWheels)
{
    // 타이어 마찰력 테이블 (예시 - 실제 값은 튜닝 필요)
    const PxU32 NUM_FRICTION_PAIRS = 4;
    PxReal tireFrictionData[2 * NUM_FRICTION_PAIRS] = {
        // x0, y0, x1, y1, ...
        0.0f, 1.0f,   // 스립 0일 때 마찰계수 1.0
        0.1f, 0.95f,
        0.2f, 0.9f,
        1.0f, 0.7f    // 스립 1.0일 때 마찰계수 0.7
    };
    
    // 기본 휠 데이터 (모든 휠에 동일하게 적용 예시)
    PxVehicleWheelData wheelData;
    wheelData.mRadius = 0.3f;
    wheelData.mWidth = 0.2f;
    wheelData.mMass = 20.0f;
    wheelData.mMOI = 0.5f * wheelData.mMass * wheelData.mRadius * wheelData.mRadius;
    wheelData.mDampingRate = 0.25f;
    wheelData.mMaxSteer = PxPi * 0.3333f; // 예: 최대 조향각 (앞바퀴용)
    wheelData.mMaxBrakeTorque = 1500.0f;
    wheelData.mMaxHandBrakeTorque = 3000.0f; // 예: 핸드브레이크 토크 (뒷바퀴용)
    wheelData.mToeAngle = 0.0f;

    // 기본 타이어 데이터
    PxVehicleTireData tireData;
    tireData.mLatStiffX = 2.0f;
    tireData.mLatStiffY = 18.0f;
    tireData.mLongitudinalStiffnessPerUnitGravity = 1000.0f;
    tireData.mCamberStiffnessPerUnitGravity = 0.0f; // 캠버각 효과 (보통 0)
    // tireData.mFrictionVsSlipGraph[0][0] = 0.0f;  tireData.mFrictionVsSlipGraph[0][1] = 0.8f;  // 예: 정지 시 마찰 0.8
    // tireData.mFrictionVsSlipGraph[1][0] = 0.2f;  tireData.mFrictionVsSlipGraph[1][1] = 1.0f;  // 예: 최대 마찰 1.0
    // tireData.mFrictionVsSlipGraph[2][0] = 1.0f;  tireData.mFrictionVsSlipGraph[2][1] = 0.7f;  // 예: 미끄러질 때 마찰 0.7
    tireData.mType = 0; // 기본 타이어 타입 ID

    // 기본 서스펜션 데이터
    PxVehicleSuspensionData suspData;
    suspData.mMaxCompression = 0.2f;
    suspData.mMaxDroop = 0.2f;
    suspData.mSpringStrength = 35000.0f;
    suspData.mSpringDamperRate = 4500.0f;
    suspData.mSprungMass = 1500.0f / numWheels; // 예시: 차량 질량을 휠 개수로 나눈 값 (튜닝 필요!)

    // Z-up, X-Forward 기준 휠 오프셋 (예시, 차량 모델에 맞게 조정)
    // FL, FR, RL, RR 순서
    PxVec3 wheelCenterActorOffsets[4];
    if (numWheels >= 4)
    {
        wheelCenterActorOffsets[PxVehicleDrive4WWheelOrder::eFRONT_LEFT]  = PxVec3( 1.2f,  0.8f, -0.1f); // Z값을 휠 반경 고려하여 조정
        wheelCenterActorOffsets[PxVehicleDrive4WWheelOrder::eFRONT_RIGHT] = PxVec3( 1.2f, -0.8f, -0.1f);
        wheelCenterActorOffsets[PxVehicleDrive4WWheelOrder::eREAR_LEFT]   = PxVec3(-1.2f,  0.8f, -0.1f);
        wheelCenterActorOffsets[PxVehicleDrive4WWheelOrder::eREAR_RIGHT]  = PxVec3(-1.2f, -0.8f, -0.1f);
    }

    PxVec3 suspTravelDir(0.0f, 0.0f, -1.0f); // Z축 아래 방향

    // 레이캐스트 필터 데이터 (지면의 queryFilterData.word3와 일치하도록)
    PxFilterData qryFilterData;
    qryFilterData.word3 = 0; // 예시: 기본 지면 타입 ID

    for (PxU32 i = 0; i < numWheels; ++i)
    {
        wheelsSimData.setWheelData(i, wheelData);
        // 앞바퀴와 뒷바퀴의 조향/핸드브레이크 설정을 다르게 할 수 있음
        if (i >= 2)  // 뒷바퀴
        {
            PxVehicleWheelData rearWheelData = wheelData;
            rearWheelData.mMaxSteer = 0.0f; // 뒷바퀴는 조향 안 함
            wheelsSimData.setWheelData(i, rearWheelData);
        }
        else // 앞바퀴
        { 
            PxVehicleWheelData frontWheelData = wheelData;
            frontWheelData.mMaxHandBrakeTorque = 0.0f; // 앞바퀴는 핸드브레이크 없음 (일반적)
            wheelsSimData.setWheelData(i, frontWheelData);
        }

        wheelsSimData.setTireData(i, tireData);
        wheelsSimData.setSuspensionData(i, suspData);
        if (i < 4)
        {
            wheelsSimData.setWheelCentreOffset(i, wheelCenterActorOffsets[i]);
            wheelsSimData.setSuspForceAppPointOffset(i, wheelCenterActorOffsets[i]);
        }
        
        wheelsSimData.setSuspTravelDirection(i, suspTravelDir);
        wheelsSimData.setWheelShapeMapping(i, i);
        wheelsSimData.setSceneQueryFilterData(i, qryFilterData);
    }

    // 4륜 구동 차량의 경우, 어떤 휠이 어떤 디퍼렌셜에 연결되는지 설정
    if (numWheels >= 4)
    {
        wheelsSimData.setWheelShapeMapping(PxVehicleDrive4WWheelOrder::eFRONT_LEFT, PxVehicleDrive4WWheelOrder::eFRONT_LEFT);
        wheelsSimData.setWheelShapeMapping(PxVehicleDrive4WWheelOrder::eFRONT_RIGHT, PxVehicleDrive4WWheelOrder::eFRONT_RIGHT);
        wheelsSimData.setWheelShapeMapping(PxVehicleDrive4WWheelOrder::eREAR_LEFT, PxVehicleDrive4WWheelOrder::eREAR_LEFT);
        wheelsSimData.setWheelShapeMapping(PxVehicleDrive4WWheelOrder::eREAR_RIGHT, PxVehicleDrive4WWheelOrder::eREAR_RIGHT);
    }
}

// 구동계 데이터 설정 (실제로는 SnippetVehicleCreate.cpp의 createDriveSimData4W 참고)
static void setupDefaultDriveSimData4W(PxVehicleDriveSimData4W& driveSimData, const PxVehicleWheelsSimData& wheelsSimData)
{
    // 엔진 토크 커브 (예시 - 실제 값은 튜닝 필요)
    PxFixedSizeLookupTable<PxVehicleEngineData::eMAX_NB_ENGINE_TORQUE_CURVE_ENTRIES> engineTorqueTable;
    engineTorqueTable.addPair(0.0f, 0.8f);    // (RPM 비율, 토크 비율) - 아이들 시 토크
    engineTorqueTable.addPair(0.33f, 1.0f);   // 최대 토크 지점
    engineTorqueTable.addPair(0.66f, 0.9f);
    engineTorqueTable.addPair(1.0f, 0.7f);    // 최대 RPM 시 토크
    
    PxVehicleEngineData engineData;
    engineData.mMOI = 1.0f;
    engineData.mPeakTorque = 500.0f; // Nm
    engineData.mMaxOmega = 600.0f;   // rad/s (약 5700 RPM)
    engineData.mDampingRateFullThrottle = 0.15f;
    engineData.mDampingRateZeroThrottleClutchEngaged = 0.4f;
    engineData.mDampingRateZeroThrottleClutchDisengaged = 0.1f;
    engineData.mTorqueCurve = engineTorqueTable; // 위에서 정의한 토크 커브 설정
    driveSimData.setEngineData(engineData);

    PxVehicleGearsData gearsData;
    gearsData.mSwitchTime = 0.25f; // 기어 변속 시간
    gearsData.mRatios[PxVehicleGearsData::eREVERSE] = -4.0f;
    gearsData.mRatios[PxVehicleGearsData::eNEUTRAL] = 0.0f;
    gearsData.mRatios[PxVehicleGearsData::eFIRST] = 4.0f;
    gearsData.mRatios[PxVehicleGearsData::eSECOND] = 2.5f;
    gearsData.mRatios[PxVehicleGearsData::eTHIRD] = 1.8f;
    gearsData.mRatios[PxVehicleGearsData::eFOURTH] = 1.3f;
    gearsData.mFinalRatio = 4.0f; // 최종 감속비
    driveSimData.setGearsData(gearsData);

    // 자동 변속기 설정 (setUseAutoGears(true) 사용 시)
    PxVehicleAutoBoxData autoBoxData;
    autoBoxData.setLatency(0.5f); // 기어 변경 후 다음 자동 변경까지 최소 시간 (초)
    // 각 기어에서 다음 기어로 올라가는 RPM 비율 (엔진 최대 RPM 대비)
    autoBoxData.mUpRatios[PxVehicleGearsData::eFIRST] = 0.65f;
    autoBoxData.mUpRatios[PxVehicleGearsData::eSECOND] = 0.70f;
    autoBoxData.mUpRatios[PxVehicleGearsData::eTHIRD] = 0.75f;
    // 각 기어에서 이전 기어로 내려가는 RPM 비율
    autoBoxData.mDownRatios[PxVehicleGearsData::eSECOND] = 0.50f;
    autoBoxData.mDownRatios[PxVehicleGearsData::eTHIRD] = 0.55f;
    autoBoxData.mDownRatios[PxVehicleGearsData::eFOURTH] = 0.60f;
    driveSimData.setAutoBoxData(autoBoxData);

    PxVehicleClutchData clutchData;
    clutchData.mStrength = 10.0f;
    driveSimData.setClutchData(clutchData);

    PxVehicleAckermannGeometryData ackermannData;
    // WheelsSimData에서 휠 오프셋 정보를 가져와서 설정
    if (wheelsSimData.getNbWheels() >= 4) {
        const PxVec3 flOffset = wheelsSimData.getWheelCentreOffset(PxVehicleDrive4WWheelOrder::eFRONT_LEFT);
        const PxVec3 frOffset = wheelsSimData.getWheelCentreOffset(PxVehicleDrive4WWheelOrder::eFRONT_RIGHT);
        const PxVec3 rlOffset = wheelsSimData.getWheelCentreOffset(PxVehicleDrive4WWheelOrder::eREAR_LEFT);
        // Z-up, X-Forward 기준
        ackermannData.mAxleSeparation = PxAbs(flOffset.x - rlOffset.x); // 앞뒤 축간 거리 (X축 차이)
        ackermannData.mFrontWidth = PxAbs(flOffset.y - frOffset.y);   // 앞바퀴 좌우 폭 (Y축 차이)
        ackermannData.mRearWidth = PxAbs(wheelsSimData.getWheelCentreOffset(PxVehicleDrive4WWheelOrder::eREAR_LEFT).y - wheelsSimData.getWheelCentreOffset(PxVehicleDrive4WWheelOrder::eREAR_RIGHT).y);
    }
    ackermannData.mAccuracy = 1.0f; // 애커먼 조향 정확도 (0~1)
    driveSimData.setAckermannGeometryData(ackermannData);

    PxVehicleDifferential4WData diffData;
    diffData.mType = PxVehicleDifferential4WData::eDIFF_TYPE_LS_4WD; // 예: 리미티드 슬립 4륜 구동
    diffData.mFrontRearSplit = 0.45f;  // 앞 45%, 뒤 55% 토크 분배
    diffData.mFrontLeftRightSplit = 0.5f; // 앞축 좌우 50:50
    diffData.mRearLeftRightSplit = 0.5f;  // 뒷축 좌우 50:50
    diffData.mCentreBias = 1.3f;       // 중앙 디퍼렌셜 바이어스
    diffData.mFrontBias = 1.3f;        // 앞축 디퍼렌셜 바이어스
    diffData.mRearBias = 1.3f;         // 뒷축 디퍼렌셜 바이어스
    driveSimData.setDiffData(diffData);
}

bool VehicleManager::CreatePlayerVehicle(const PxTransform& initialPose)
{
    if (!WheelsSimData || !PxPhysics || !DefaultMaterial || !PxScene)
        return false;
    
    // 1. 차량 데이터 설정 (WheelsSimData, DriveSimData4W 채우기)
    // SnippetVehicleCreate.cpp의 createWheelsSimData, createDriveSimData4W 로직 참고
    setupDefaultWheelsSimData(*WheelsSimData, NumWheelsPerVehicle);
    setupDefaultDriveSimData4W(DriveSimData4W, *WheelsSimData);                    

    // 2. 섀시 데이터 준비
    physx::PxVehicleChassisData chassisData = createChassisData();

    // 3. 섀시 액터 생성 (SnippetVehicleCreate.cpp의 createChassis 참고)
    // 주의: Snippet은 Y-up 기준. Z-up으로 변환 필요.
    // 예: chassisDims(2.5f,2.0f,5.0f) -> PxBoxGeometry(5.0f*0.5f, 2.5f*0.5f, 2.0f*0.5f) (X,Y,Z 순서 가정)
    physx::PxBoxGeometry chassisGeom(4.5f * 0.5f, 2.0f * 0.5f, 1.4f * 0.5f); // Z-up, X-Forward 기준 섀시 half-extents (길이, 폭, 높이)
    
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
    sqDesc.preFilterShader  = snippetvehicle::WheelSceneQueryPreFilterBlocking;
    sqDesc.postFilterShader = snippetvehicle::WheelSceneQueryPostFilterBlocking;
    
    sqDesc.queryMemory.userRaycastResultBuffer = SuspensionRaycastData.sqResults;
    sqDesc.queryMemory.userRaycastTouchBuffer = SuspensionRaycastData.sqHitBuffer;
    sqDesc.queryMemory.raycastTouchBufferSize = SuspensionRaycastData.nbSqResults;
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
