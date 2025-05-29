#include "AVehicleActor.h"
#include <vehicle/PxVehicleDrive4W.h>
#include <PxPhysicsAPI.h> 
#include "EditorEngine.h"
#include "Components/PrimitiveComponents/MeshComponents/StaticMeshComponents/StaticMeshComponent.h"
#include "Engine/FEditorStateManager.h"
#include "Engine/World.h"
#include "Physics/Car/VehicleManager.h"
#include "Physics/Car/TransformConvert.h"
#include "Components/InputComponent.h"

AVehicleActor::AVehicleActor()
{
    StaticMeshComponent = AddComponent<UStaticMeshComponent>(EComponentOrigin::Constructor);
    RootComponent = StaticMeshComponent;
    UEditorEngine* EditorEngine = Cast<UEditorEngine>(GEngine);
    VehicleManager = EditorEngine->EditorWorldContext->GetWorld()->VehicleManager;
}
AVehicleActor::AVehicleActor(const AVehicleActor& Other) : Super(Other)
{
}

// Vehicle에 맞게 Desc 생성하는 함수
snippetvehicle::VehicleDesc AVehicleActor::CreateDesc()
{
    snippetvehicle::VehicleDesc VehicleDesc;

    // Body 컴포넌트의 로컬 AABB 가져오기
    FBoundingBox BodyAABB = StaticMeshComponent->GetBoundingBox();  
    FVector LocalMin = BodyAABB.min;
    FVector LocalMax = BodyAABB.max;

    // 8개 코너 계산
    FVector Corners[8] = {
        {LocalMin.X, LocalMin.Y, LocalMin.Z},
        {LocalMin.X, LocalMin.Y, LocalMax.Z},
        {LocalMin.X, LocalMax.Y, LocalMin.Z},
        {LocalMin.X, LocalMax.Y, LocalMax.Z},
        {LocalMax.X, LocalMin.Y, LocalMin.Z},
        {LocalMax.X, LocalMin.Y, LocalMax.Z},
        {LocalMax.X, LocalMax.Y, LocalMin.Z},
        {LocalMax.X, LocalMax.Y, LocalMax.Z},
    };

    // 월드 트랜스폼 정보
    FVector WorldLoc   = StaticMeshComponent->GetWorldLocation();
    FQuat   WorldQuat  = StaticMeshComponent->GetWorldRotation().ToQuaternion();
    FVector WorldScale = StaticMeshComponent->GetWorldScale();

    // AABB를 월드로 변환하여 월드 최소/최대점 계산
    FVector WorldMin(FLT_MAX), WorldMax(-FLT_MAX);
    for (int i = 0; i < 8; ++i)
    {
        // 스케일 → 회전 → 이동
        FVector Scaled = Corners[i] * WorldScale;
        FVector Rotated = WorldQuat.RotateVector(Scaled);
        FVector W = WorldLoc + Rotated;
        WorldMin.X = FMath::Min(WorldMin.X, W.X);
        WorldMin.Y = FMath::Min(WorldMin.Y, W.Y);
        WorldMin.Z = FMath::Min(WorldMin.Z, W.Z);
        WorldMax.X = FMath::Max(WorldMax.X, W.X);
        WorldMax.Y = FMath::Max(WorldMax.Y, W.Y);
        WorldMax.Z = FMath::Max(WorldMax.Z, W.Z);
    }

    // 월드 AABB에서 중심과 반치수 계산 (m 단위로 변환)
    auto CmToM = [](float x){ return x * 0.01f; };
    FVector CenterWS   = (WorldMin + WorldMax) * 0.5f;
    FVector HalfExtentsWS = (WorldMax - WorldMin) * 0.5f;

    // PhysX용 VehicleDesc 세팅
    VehicleDesc.chassisDims       = PxVec3( CmToM(HalfExtentsWS.X*2),
                                            CmToM(HalfExtentsWS.Y*2),
                                            CmToM(HalfExtentsWS.Z*2) );
    VehicleDesc.chassisCMOffset   = PxVec3( CmToM(CenterWS.X - GetActorLocation().X),
                                            CmToM(CenterWS.Y - GetActorLocation().Y),
                                            CmToM(CenterWS.Z - GetActorLocation().Z) );
    // 밀도 500kg/m^3 기준 질량 & 관성 예시
    float Volume = VehicleDesc.chassisDims.x
                 * VehicleDesc.chassisDims.y
                 * VehicleDesc.chassisDims.z;
    VehicleDesc.chassisMass      = 500.0f * Volume;
    float m = VehicleDesc.chassisMass;
    float x = VehicleDesc.chassisDims.x,
          y = VehicleDesc.chassisDims.y,
          z = VehicleDesc.chassisDims.z;
    VehicleDesc.chassisMOI      = PxVec3(
        1/12.0f * m * (y*y + z*z),
        1/12.0f * m * (x*x + z*z),
        1/12.0f * m * (x*x + y*y)
    );

    // Tires
    TArray<USceneComponent*> Children = StaticMeshComponent->GetAttachChildren();
    int32 NumWheels = Children.Num();
    VehicleDesc.numWheels = NumWheels;
    VehicleDesc.wheelMass = 20.0f;   // 또는 AABB 기반 계산
    VehicleDesc.wheelOffsets.SetNum(NumWheels);

    for (int32 i = 0; i < NumWheels; ++i)
    {
        // 로컬 AABB 가져오기
        FBoundingBox TireAABB = Cast<USceneComponent>(Children[i])->GetBoundingBox();
        FVector tMin = TireAABB.min, tMax = TireAABB.max;
        // 8개 모서리 → 월드로 변환 → 월드 AABB 계산 코드(생략)

        // 편의상 대표 타이어 반치수만 뽑아서
        if (i == 0)
        {
            float r_local = (tMax.Z - tMin.Z)*0.5f;  // Z축 기준
            float w_local = (tMax.Y - tMin.Y);      // Y축 기준 폭
            VehicleDesc.wheelRadius = CmToM(r_local * WorldScale.Z);
            VehicleDesc.wheelWidth  = CmToM(w_local * WorldScale.Y);
        }

        // 바퀴 위치(Local → World 오프셋)
        FVector RelLoc = Children[i]->GetRelativeLocation();
        VehicleDesc.wheelOffsets[i] = PxVec3(
            CmToM(RelLoc.X),
            CmToM(RelLoc.Y),
            CmToM(RelLoc.Z)
        );
    }

    VehicleDesc.wheelMOI = 0.5f * VehicleDesc.wheelMass * VehicleDesc.wheelRadius * VehicleDesc.wheelRadius;

    // Material
    PxMaterial* DefaultMat = gMaterial;
    DefaultMat->setRestitution(0.0f);  // 반발계수
    DefaultMat->setStaticFriction(1.0f);
    DefaultMat->setDynamicFriction(1.0f);  // friction을 1.0 이상으로 올려 주면, 지면에 달라붙듯이 미끄러짐도 억제
    VehicleDesc.chassisMaterial = DefaultMat;
    VehicleDesc.wheelMaterial   = DefaultMat;

    // FilterData
    VehicleDesc.chassisSimFilterData = FPhysX::MakeFilterData(FPhysX::ECollisionGroup::VehicleBody, FPhysX::ECollisionGroup::Environment);
    VehicleDesc.wheelSimFilterData = FPhysX::MakeFilterData(FPhysX::ECollisionGroup::VehicleWheel, FPhysX::ECollisionGroup::Environment);

    // UserData
    VehicleDesc.actorUserData = new snippetvehicle::ActorUserData();
    VehicleDesc.shapeUserDatas = new snippetvehicle::ShapeUserData[1 + VehicleDesc.numWheels];
    
    return VehicleDesc;
}

// chassis, wheels 생성
void AVehicleActor::CreateVehicle()
{
    for (USceneComponent* Child : StaticMeshComponent->GetAttachChildren())
    {
        if (auto Tire = Cast<UStaticMeshComponent>(Child))
        {
            TireMeshes.Add(Tire);
        }
    }
    
    // Desc 준비
    snippetvehicle::VehicleDesc Desc = CreateDesc();

    PxPhysics* physics = gPhysics;
    PxCooking* cooking = gCooking;

    // (a) 차체용 ConvexMesh 준비
    PxConvexMesh* chassisConvex = snippetvehicle::createChassisMesh(
        Desc.chassisDims, *physics, *cooking);

    // (b) 바퀴용 ConvexMesh 배열 & Material 배열 준비
    std::vector<PxConvexMesh*> wheelMeshes(Desc.numWheels);
    std::vector<PxMaterial*>    wheelMats  (Desc.numWheels);
    for (PxU32 i = 0; i < Desc.numWheels; ++i)
    {
        wheelMeshes[i] = snippetvehicle::createWheelMesh(
            Desc.wheelWidth, Desc.wheelRadius, *physics, *cooking);
        wheelMats  [i] = Desc.wheelMaterial;
    }

    // (c) createVehicleActor 호출
    PxVehicleChassisData chassisData;
    chassisData.mMass     = Desc.chassisMass;      // kg
    chassisData.mMOI      = Desc.chassisMOI;       // kg·m²
    chassisData.mCMOffset = Desc.chassisCMOffset;  // m
    
    PxRigidDynamic* chassisActor = snippetvehicle::createVehicleActor(
        /*chassisData=*/ chassisData,
        /*wheelMaterials=*/ wheelMats.data(),
        /*wheelConvexes=*/ wheelMeshes.data(),
        /*numWheels=*/ Desc.numWheels,
        /*wheelFilter=*/ Desc.wheelSimFilterData,
        /*chassisMaterials=*/ &Desc.chassisMaterial,
        /*chassisConvexes=*/ &chassisConvex,
        /*numChassisMeshes=*/ 1,
        /*chassisFilter=*/ Desc.chassisSimFilterData,
        *physics
    );

    // (d) 차량 시뮬레이터 객체 생성
    PxVehicleDrive4W* vehicle4W = createVehicle4W(Desc, physics, cooking);
    PxVehicleWheels& wheels = *vehicle4W;
    PxVehicleWheelsSimData& wheelsSimData = wheels.mWheelsSimData;
    for (PxU32 i = 0; i < wheelsSimData.getNbWheels(); ++i)
    {
        PxVehicleSuspensionData susp = wheelsSimData.getSuspensionData(i);

        // ▷ 서스펜션 이동 범위 압축/드룹 한계 낮춰서 뒤틀림 억제
        susp.mMaxCompression = 0.3f;  // 기본값보다 작게
        susp.mMaxDroop       = 0.1f;  // 기본값보다 작게
        
        // ▷ 스프링·댐퍼 강화 (진동 흡수 & 몸체 롤 억제)
        susp.mSpringStrength   = 35000.0f;  // 스프링 강성 (kg/s²) — 높을수록 탄탄해지고 뭉툭하게 튕깁니다.
        susp.mSpringDamperRate = 4500.0f;  // 감쇠 계수 (kg/s) — 높을수록 진동 흡수 빨라지고, 바운스가 감소합니다.
        
        wheelsSimData.setSuspensionData(i, susp);
    }

    // ▷ 차체 Center-of-Mass를 좀 더 아래로 내리기
    chassisData.mCMOffset.z -= 0.2f;  // 20cm 정도 더 낮춰 줍니다.

    // (e) UserData 연결
    Desc.actorUserData->actor   = chassisActor;
    Desc.actorUserData->vehicle = vehicle4W;
    snippetvehicle::configureUserData(
        vehicle4W, Desc.actorUserData, Desc.shapeUserDatas);

    // (f) PhysX 씬에 추가
    VehicleManager->GetPxScene()->addActor(*chassisActor);
    VehicleManager->RegisterPlayerVehicle(chassisActor, vehicle4W, Desc.numWheels);

    // (g) 월드 변환 반영
    FVector WorldLoc = StaticMeshComponent->GetWorldLocation();
    FQuat   WorldRot = StaticMeshComponent->GetWorldRotation().ToQuaternion();
    PxTransform PxPose(U2PVector(WorldLoc), U2PQuat(WorldRot));
    chassisActor->setGlobalPose(PxPose, true);

    // (h) VehicleManager 내부 인스턴스에 등록
    auto VehicleInstance = VehicleManager->GetPlayerVehicle();
    VehicleInstance = new struct VehicleInstance();
    VehicleInstance->chassisActor = chassisActor;
    VehicleInstance->vehicle      = vehicle4W;
    PxDefaultAllocator Allocator;
    VehicleInstance->setupWheelQueryResults(Desc.numWheels, Allocator);

    VehicleManager->RegisterPlayerVehicle(chassisActor, vehicle4W, Desc.numWheels);
    VehicleManager->AddVehicleActor(this);
}

void AVehicleActor::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
    Super::SetupPlayerInputComponent(PlayerInputComponent);;
    
    PlayerInputComponent->BindAxis("MoveForward", [this](float Value){ this->MoveForward(Value); });
    PlayerInputComponent->BindAxis("MoveRight", [this](float Value){ this->MoveRight(Value); });
    PlayerInputComponent->BindAction("HandBrake", [this](){ this->OnHandBrakePressed(); });
    PlayerInputComponent->BindAction("HandBrake", [this](){ this->OnHandBrakeReleased(); });
}

void AVehicleActor::MoveForward(float Value)
{
    // W: Value>0 이면 가속, S: Value<0 이면 브레이크
    Throttle = Value > 0.f ? Value : 0.f;
    Brake    = Value < 0.f ? -Value   : 0.f;
}

void AVehicleActor::MoveRight(float Value)
{
    // 왼/오 조향 (-1..1)
    Steering = Value;
}

void AVehicleActor::OnHandBrakePressed()
{
    bHandBrake = true;
}

void AVehicleActor::OnHandBrakeReleased()
{
    bHandBrake = false;
}