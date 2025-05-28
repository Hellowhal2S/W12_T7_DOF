#pragma once
#include "Physics/PhysX.h"
#include <vehicle/PxVehicleDrive4W.h>
#include <vehicle/PxVehicleUtil.h>

class AVehicleActor;
// SnippetVehicleSceneQuery.h 와 유사한 기능을 할 구조체/클래스
// 여기서는 Snippet의 VehicleSceneQueryData를 직접 사용하지 않고, 필요한 멤버만 가져온다고 가정
struct VehicleSuspensionRaycastData
{
    PxRaycastQueryResult* sqResults = nullptr;
    PxRaycastHit* sqHitBuffer = nullptr;
    PxU32 nbSqResults = 0;

    void allocate(PxU32 maxWheels, PxDefaultAllocator& allocator) {
        nbSqResults = maxWheels;
        sqResults = (PxRaycastQueryResult*)allocator.allocate(sizeof(PxRaycastQueryResult)*nbSqResults, "RaycastResults", __FILE__, __LINE__);
        sqHitBuffer = (PxRaycastHit*)allocator.allocate(sizeof(PxRaycastHit)*nbSqResults, "RaycastHits", __FILE__, __LINE__);
    }
    void free(PxDefaultAllocator& allocator) {
        if (sqHitBuffer) allocator.deallocate(sqHitBuffer);
        sqHitBuffer = nullptr;
        if (sqResults) allocator.deallocate(sqResults);
        sqResults = nullptr;
        nbSqResults = 0;
    }
};

struct VehicleInstance
{
    PxRigidDynamic* chassisActor = nullptr;
    PxVehicleDrive4W* vehicle = nullptr;
    PxVehicleDrive4WRawInputData inputData;
    bool isInAir = true;

    // 차량별 Wheel 쿼리 결과
    PxVehicleWheelQueryResult wheelQueryResults;
    PxWheelQueryResult* actualWheelQueryResultsBuffer = nullptr;  // 실제 데이터 저장 버퍼

    void setupWheelQueryResults(PxU32 numWheels, PxDefaultAllocator& allocator) {
        actualWheelQueryResultsBuffer = (PxWheelQueryResult*)allocator.allocate(sizeof(PxWheelQueryResult)*numWheels, "WheelQueryResults", __FILE__, __LINE__);
        wheelQueryResults.nbWheelQueryResults = numWheels;
        wheelQueryResults.wheelQueryResults = actualWheelQueryResultsBuffer;
    }
    void releaseWheelQueryResults(PxDefaultAllocator& allocator) {
        if (actualWheelQueryResultsBuffer) allocator.deallocate(actualWheelQueryResultsBuffer);
        actualWheelQueryResultsBuffer = nullptr;
        wheelQueryResults.nbWheelQueryResults = 0;
        wheelQueryResults.wheelQueryResults = nullptr;
    }
};

class VehicleManager
{
public:
    VehicleManager(PxScene* scene);
    ~VehicleManager();

    int GetVehicleActorSize() { return VehicleActors.Num(); }
    void AddVehicleActor(AVehicleActor* Vehicle) { VehicleActors.Add(Vehicle); }
    
    PxScene* GetPxScene() { return PxScene; }
    VehicleInstance* GetPlayerVehicle() { return PlayerVehicleInstance.vehicle ? &PlayerVehicleInstance : nullptr; }

    void UpdateAllVehicles(float dt);
    void UpdatePlayerVehicleInput(float accel, float brake, float steer, bool handbrake);
    
private:
    PxScene* PxScene = nullptr;
    PxPhysics* PxPhysics = nullptr;
    PxCooking* PxCooking = nullptr;
    PxMaterial* DefaultMaterial = nullptr;

    // 차량 설정 데이터
    PxVehicleWheelsSimData* WheelsSimData = nullptr;
    PxVehicleDriveSimData4W DriveSimData4W;

    // 생성된 차량 인스턴스 관리
    VehicleInstance PlayerVehicleInstance;

    // 서스펜션 레이캐스트 관련
    PxBatchQuery* BatchQuery = nullptr;
    VehicleSuspensionRaycastData SuspensionRaycastData;
    static const PxU32 NumWheelsPerVehicle = 4;

    // 입력 스무딩 및 조향 테이블
    PxFixedSizeLookupTable<8> SteerVsForwardSpeedTable;
    PxVehicleKeySmoothingData KeySmoothingData;
    bool MimicKeyInputs = false;

    TArray<AVehicleActor*> VehicleActors;
};