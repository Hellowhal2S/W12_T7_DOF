#include "World.h"

#include "Renderer/Renderer.h"
#include "PlayerCameraManager.h"
#include "BaseGizmos/TransformGizmo.h"
#include "Camera/CameraComponent.h"
#include "LevelEditor/SLevelEditor.h"
#include "Engine/FLoaderOBJ.h"
#include "UObject/UObjectIterator.h"
#include "Level.h"
#include "Engine/FBXLoader.h"
#include "Actors/ADodge.h"
#include "Animation/Skeleton.h"
#include "Components/PrimitiveComponents/MeshComponents/SkeletalMeshComponent.h"
#include "Contents/GameManager.h"
#include "Serialization/FWindowsBinHelper.h"

#include "GameFramework//PlayerController.h"
#include "GameFramework/Character.h"
#include "Script/LuaManager.h"
#include "UObject/UObjectArray.h"
#include "UnrealEd/PrimitiveBatch.h"

UWorld::UWorld(const UWorld& Other): UObject(Other)
                                   , defaultMapName(Other.defaultMapName)
                                   , Level(Other.Level)
                                   , WorldType(Other.WorldType)
                                   , LocalGizmo(nullptr)
{
}

void UWorld::InitWorld()
{
    // TODO: Load Scene
    if (Level == nullptr)
    {
        Level = FObjectFactory::ConstructObject<ULevel>(this);
    }
    PreLoadResources();

    // PhysicsScene 초기화
    InitPhysicsScene();
    
    if (WorldType == EWorldType::Editor)
    {
        LoadScene("NewScene.scene");
    }
    else
    {
        CreateBaseObject(WorldType);
    }
}

void UWorld::InitPhysicsScene()
{
    PxSceneDesc sceneDesc(gPhysics->getTolerancesScale());
    sceneDesc.gravity = PxVec3(0, 0, -9.81f);
    gDispatcher = PxDefaultCpuDispatcherCreate(4);
    sceneDesc.cpuDispatcher = gDispatcher;
    sceneDesc.flags |= PxSceneFlag::eENABLE_ACTIVE_ACTORS;
    sceneDesc.flags |= PxSceneFlag::eENABLE_CCD;
    sceneDesc.flags |= PxSceneFlag::eENABLE_PCM;;
    sceneDesc.filterShader = FPhysX::MyFilterShader;
    gMyCallback = new MySimulationEventCallback();
    sceneDesc.simulationEventCallback = gMyCallback;
    gScene = gPhysics->createScene(sceneDesc);
        
    PxPvdSceneClient* PvdClient = gScene->getScenePvdClient();
    if (PvdClient)
    {
        PvdClient->setScenePvdFlag(PxPvdSceneFlag::eTRANSMIT_CONSTRAINTS, true);
        PvdClient->setScenePvdFlag(PxPvdSceneFlag::eTRANSMIT_CONTACTS, true);
        PvdClient->setScenePvdFlag(PxPvdSceneFlag::eTRANSMIT_SCENEQUERIES, true);
    }

    PxPvdSceneClient* pvdClient = gScene->getScenePvdClient();
        
    PxRigidStatic* GroundPlane = PxCreatePlane(*gPhysics, PxPlane(0, 0, 1, 0), *gMaterial);
    PxShape* planeShape;
    GroundPlane->getShapes(&planeShape, 1);
    planeShape->setSimulationFilterData(FPhysX::MakeFilterData(FPhysX::ECollisionGroup::Environment, FPhysX::ECollisionGroup::All));
    GroundPlane->setName("GroundPlane");
    gScene->addActor(*GroundPlane);

    // FGameObject FallingBox = CreateBox(PxVec3(0, 0, 100), PxVec3(1, 1, 1));
    // if (auto* rigidDynamic = FallingBox.rigidBody->is<physx::PxRigidDynamic>())
    // {
    //     rigidDynamic->setAngularDamping(0.5f);
    //     rigidDynamic->setLinearDamping (0.5f);
    //     rigidDynamic->setMass          (10.0f);
    // }
    // FallingBox.rigidBody->setName("FallingBox");
    // FallingBox.scene = gScene;
    // gObjects.push_back(FallingBox);
    //     
    // FGameObject BigBox = CreateBox(PxVec3(0, 0, 1),
    //                                PxVec3(100, 100, 1),
    //                                FPhysX::EActorType::Static);
    // if (auto* rigidDynamic = BigBox.rigidBody->is<physx::PxRigidDynamic>())
    // {
    //     rigidDynamic->setAngularDamping(0.5f);
    //     rigidDynamic->setLinearDamping (0.5f);
    //     rigidDynamic->setMass          (10.0f);
    // }
    // BigBox.rigidBody->setName("BigBox");
    // BigBox.scene = gScene;
    // gObjects.push_back(BigBox);
    //
    // FGameObject TriggerBox = CreateBox(
    //             PxVec3(0, 0, 20),
    //         PxVec3(5, 5, 1),
    //                 FPhysX::EActorType::Static,
    //                 PxShapeFlag::eTRIGGER_SHAPE | PxShapeFlag::eSCENE_QUERY_SHAPE);
    // TriggerBox.rigidBody->setName("TriggerBox");
    // TriggerBox.scene = gScene;
    // gObjects.push_back(TriggerBox);

    printf("Init Physics Scene\n");
}

FGameObject UWorld::CreateBox(
    const PxVec3&     pos,
    const PxVec3&     halfExtents,
    FPhysX::EActorType actorType,
    const PxShapeFlags& shapeFlags
) const
{
    FGameObject obj;
    PxTransform pose(pos);
    PxRigidActor* actor = nullptr;
    if (actorType == FPhysX::EActorType::Dynamic)
        actor = gPhysics->createRigidDynamic(pose);
    else
        actor = gPhysics->createRigidStatic (pose);
    obj.rigidBody = actor;
    obj.scene = gScene;
    
    PxShape* shape = gPhysics->createShape(PxBoxGeometry(halfExtents), *gMaterial);
    shape->setFlags(shapeFlags);
    PxFilterData filterData = FPhysX::MakeFilterData(FPhysX::ECollisionGroup::BoxCollider, FPhysX::ECollisionGroup::All);
    shape->setSimulationFilterData(filterData);
    obj.rigidBody->attachShape(*shape);

    if (actorType == FPhysX::EActorType::Dynamic)
    {
        if (auto* rigidDynamic = obj.rigidBody->is<physx::PxRigidDynamic>())
        {
            // 질량·관성 업데이트
            PxRigidBodyExt::updateMassAndInertia(*rigidDynamic, 10.0f);

            // onWake/onSleep 알림 켜기
            rigidDynamic->setActorFlag(PxActorFlag::eSEND_SLEEP_NOTIFIES, true);
            
            // pose integration preview(=onAdvance) 켜기
            rigidDynamic->setRigidBodyFlag(PxRigidBodyFlag::eENABLE_POSE_INTEGRATION_PREVIEW, true);
        }   
    }
    
    gScene->addActor(*obj.rigidBody);
    obj.UpdateFromPhysics();
    return obj;
}
    
void UWorld::Simulate(float dt) {
    gScene->simulate(dt);
    gScene->fetchResults(true);
    
     // 예시: 모든 동적(Dynamic) 및 정적(Static) 바디 가져오기
    PxU32 bufferSize = gScene->getNbActors(PxActorTypeFlag::eRIGID_DYNAMIC | PxActorTypeFlag::eRIGID_STATIC);
    std::vector<PxActor*> actorBuffer(bufferSize);
    gScene->getActors(PxActorTypeFlag::eRIGID_DYNAMIC | PxActorTypeFlag::eRIGID_STATIC, actorBuffer.data(), bufferSize);
    for (PxU32 i = 0; i < bufferSize; ++i) {
        PxActor* actor = actorBuffer[i];
        if (!actor->userData) continue;
    
        // actor를 사용 (예: userData로 엔진 오브젝트 찾기, pose 읽기 등)
        PxTransform pose;
        if (PxRigidDynamic* dyn = actor->is<PxRigidDynamic>()) {
            pose = dyn->getGlobalPose();
        } else if (PxRigidStatic* stat = actor->is<PxRigidStatic>()) {
            pose = stat->getGlobalPose();
        }
        
        FBodyInstance* BodyInst = (FBodyInstance*)actor->userData;
        
        // StaticMeshComponent
        if (UStaticMeshComponent* staticComp = Cast<UStaticMeshComponent>(BodyInst->OwnerComponent)) {
            staticComp->SetWorldLocation(FromPxVec3(pose.p));
            staticComp->SetWorldRotation(FromPxQuat(pose.q));
        }
        // SkeletalMeshComponent - 본 단위
        else if (USkeletalMeshComponent* skelComp = Cast<USkeletalMeshComponent>(BodyInst->OwnerComponent)) {
            int* Index = skelComp->GetSkeletalMesh()->GetSkeleton()->GetRefSkeletal()->BoneNameToIndexMap.Find(BodyInst->BoneName.ToString());
            int ParentIndex = skelComp->GetSkeletalMesh()->GetRenderData().Bones[*Index].ParentIndex;
            FMatrix RotationMatrix = FromPxQuat(pose.q).GetSafeNormal().ToMatrix();
            FMatrix TranslationMatrix = FMatrix::CreateTranslationMatrix(FromPxVec3(pose.p));
            FMatrix RefGMat;
            
            if (ParentIndex != INDEX_NONE)
            {
                RefGMat = skelComp->GetSkeletalMesh()->GetSkeleton()->GetRefSkeletal()->RawBones[ParentIndex].GlobalTransform;
            }
            FMatrix GlobalTransform = RefGMat * RotationMatrix;
            GlobalTransform.M[3][0] = TranslationMatrix.M[3][0];
            GlobalTransform.M[3][1] = TranslationMatrix.M[3][1];
            GlobalTransform.M[3][2] = TranslationMatrix.M[3][2];
            if (ParentIndex != INDEX_NONE)
            {
                int PaParentIndex = skelComp->GetSkeletalMesh()->GetRenderData().Bones[ParentIndex].ParentIndex;
                if (PaParentIndex != INDEX_NONE)
                {
                    skelComp->GetSkeletalMesh()->GetRenderData().Bones[ParentIndex].LocalTransform = GlobalTransform * skelComp->GetSkeletalMesh()->GetRenderData().Bones[PaParentIndex].GlobalTransform.Inverse();
                }
                else
                {
                    skelComp->GetSkeletalMesh()->GetRenderData().Bones[ParentIndex].LocalTransform = GlobalTransform;
                }
            }
            skelComp->GetSkeletalMesh()->UpdateBoneHierarchy();
        }
    }
}

void UWorld::LoadLevel(const FString& LevelName)
{
    // !TODO : 레벨 로드
    // 이름으로 레벨 로드한다
    // 실패 하면 현재 레벨 유지
}

void UWorld::PreLoadResources()
{
    FManagerOBJ::CreateStaticMesh(TEXT("Assets/CastleObj.obj"));
}

void UWorld::CreateBaseObject(EWorldType::Type WorldType)
{
    if (WorldType == EWorldType::PIE)
    {
        return;
    }

    if (LocalGizmo == nullptr && WorldType)
    {
        LocalGizmo = FObjectFactory::ConstructObject<UTransformGizmo>(this);
    }
}


void UWorld::ReleaseBaseObject()
{
    LocalGizmo = nullptr;
}

void UWorld::Tick(ELevelTick tickType, float deltaSeconds)
{
    if (tickType == LEVELTICK_ViewportsOnly)
    {
        if (LocalGizmo)
        {
            LocalGizmo->Tick(deltaSeconds);
        }
        
        FGameManager::Get().EditorTick(deltaSeconds);
    }
    // SpawnActor()에 의해 Actor가 생성된 경우, 여기서 BeginPlay 호출
    if (tickType == LEVELTICK_All)
    {
        FLuaManager::Get().BeginPlay();
        TSet<AActor*> PendingActors = Level->PendingBeginPlayActors;
        for (AActor* Actor : PendingActors)
        {
            Actor->BeginPlay();
            Level->PendingBeginPlayActors.Remove(Actor);
        }

        TArray CopyActors = Level->GetActors();
        for (AActor* Actor : CopyActors)
        {
            Actor->Tick(deltaSeconds);
        }

        FGameManager::Get().Tick(deltaSeconds);
    }
}

void UWorld::Release()
{
    if (WorldType == EWorldType::Editor)
    {
        SaveScene("Assets/Scenes/AutoSave.Scene");
    }
    TArray<AActor*> Actors = Level->GetActors();
    for (AActor* Actor : Actors)
    {
        Actor->Destroy();
    }
    if (LocalGizmo)
    {
        LocalGizmo->Destroy();
    }

    GUObjectArray.MarkRemoveObject(Level);
    // TODO Level -> Release로 바꾸기
    // Level->Release();
    GUObjectArray.MarkRemoveObject(this);

	pickingGizmo = nullptr;
	ReleaseBaseObject();

    for (FGameObject& obj : gObjects) obj.rigidBody->release();
    gObjects.clear();
    
    gScene->release();
    gScene = nullptr;
    
    gDispatcher->release();
    gDispatcher = nullptr;
}

void UWorld::ClearScene()
{
    // 1. PickedActor제거
    SelectedActors.Empty();
    // 2. 모든 Actor Destroy
    
    for (AActor* actor : TObjectRange<AActor>())
    {
        if (actor->GetWorld() == this)
        {
            DestroyActor(actor);
        }
    }
    Level->GetActors().Empty();
    Level->PendingBeginPlayActors.Empty();
    ReleaseBaseObject();
}

UObject* UWorld::Duplicate(UObject* InOuter)
{
    UWorld* CloneWorld = FObjectFactory::ConstructObjectFrom<UWorld>(this, InOuter);
    CloneWorld->DuplicateSubObjects(this, InOuter);
    CloneWorld->PostDuplicate();
    return CloneWorld;
}

void UWorld::DuplicateSubObjects(const UObject* SourceObj, UObject* InOuter)
{
    UObject::DuplicateSubObjects(SourceObj, InOuter);
    Level = Cast<ULevel>(Level->Duplicate(this));
    LocalGizmo = FObjectFactory::ConstructObject<UTransformGizmo>(this);
}

void UWorld::PostDuplicate()
{
    UObject::PostDuplicate();
}

UWorld* UWorld::GetWorld() const
{
    return const_cast<UWorld*>(this);
}

void UWorld::LoadScene(const FString& FileName)
{
    ClearScene(); // 기존 오브젝트 제거
    CreateBaseObject(WorldType);
    FArchive ar;
    FWindowsBinHelper::LoadFromBin(FileName, ar);

    ar >> *this;
}

void UWorld::DuplicateSelectedActors()
{
    TSet<AActor*> newSelectedActors;
    for (AActor* Actor : SelectedActors)
    {
        AActor* DupedActor = Cast<AActor>(Actor->Duplicate(this));
        FString TypeName = DupedActor->GetActorLabel().Left(DupedActor->GetActorLabel().Find("_", ESearchCase::IgnoreCase,ESearchDir::FromEnd));
        DupedActor->SetActorLabel(TypeName);
        FVector DupedLocation = DupedActor->GetActorLocation();
        DupedActor->SetActorLocation(FVector(DupedLocation.X+50, DupedLocation.Y+50, DupedLocation.Z));
        Level->GetActors().Add(DupedActor);
        Level->PendingBeginPlayActors.Add(DupedActor);
        newSelectedActors.Add(DupedActor);
    }
    SelectedActors = newSelectedActors;
}
void UWorld::DuplicateSelectedActorsOnLocation()
{
    TSet<AActor*> newSelectedActors;
    for (AActor* Actor : SelectedActors)
    {
        AActor* DupedActor = Cast<AActor>(Actor->Duplicate(this));
        FString TypeName = DupedActor->GetActorLabel().Left(DupedActor->GetActorLabel().Find("_", ESearchCase::IgnoreCase,ESearchDir::FromEnd));
        DupedActor->SetActorLabel(TypeName);
        Level->GetActors().Add(DupedActor);
        Level->PendingBeginPlayActors.Add(DupedActor);
        newSelectedActors.Add(DupedActor);
    }
    SelectedActors = newSelectedActors;
}

bool UWorld::DestroyActor(AActor* ThisActor)
{
    if (ThisActor->GetWorld() == nullptr)
    {
        return false;
    }

    if (ThisActor->IsActorBeingDestroyed())
    {
        return true;
    }

    // 액터의 Destroyed 호출
    ThisActor->Destroyed();

    if (ThisActor->GetOwner())
    {
        ThisActor->SetOwner(nullptr);
    }

    TArray<UActorComponent*> Components = ThisActor->GetComponents();
    for (UActorComponent* Component : Components)
    {
        Component->DestroyComponent();
        if (Component->IsA<USkeletalMeshComponent>())
        {
            Cast<USkeletalMeshComponent>(Component)->ReleaseBodies();
        }
        else if (Component->IsA<UStaticMeshComponent>())
        {
            Cast<UStaticMeshComponent>(Component)->ReleaseBody();
        }
    }

    // World에서 제거
    Level->GetActors().Remove(ThisActor);

    // 제거 대기열에 추가
    GUObjectArray.MarkRemoveObject(ThisActor);
    return true;
}


void UWorld::SetPickingGizmo(UObject* Object)
{
	pickingGizmo = Cast<USceneComponent>(Object);
}

void UWorld::Serialize(FArchive& ar) const
{
    int ActorCount = Level->GetActors().Num();
    ar << ActorCount;
    for (AActor* Actor : Level->GetActors())
    {
        FActorInfo ActorInfo = (Actor->GetActorInfo());
        ar << ActorInfo;
    }
}

void UWorld::Deserialize(FArchive& ar)
{
    int ActorCount;
    ar >> ActorCount;
    for (int i = 0; i < ActorCount; i++)
    {
        FActorInfo ActorInfo;
        ar >> ActorInfo;
        UClass* ActorClass = UClassRegistry::Get().FindClassByName(ActorInfo.Type);
        if (ActorClass)
        {
            AActor* Actor = SpawnActorByClass(ActorClass, this, true);
            if (Actor)
            {
                Actor->LoadAndConstruct(ActorInfo.ComponentInfos);
                Level->GetActors().Add(Actor);
            }
        }
    }
    Level->PostLoad();
    
}

/*************************임시******************************/
bool UWorld::IsPIEWorld() const
{
    return false;
}

void UWorld::BeginPlay()
{
    // FGameManager::Get().BeginPlay();

    if (PlayerController == nullptr)
    {
        PlayerController = SpawnActor<APlayerController>();

        bool bCharacterExist = false;
        for (AActor* Actor : Level->GetActors())
        {
            if (ACharacter* Character = Cast<ACharacter>(Actor))
            {
                bCharacterExist = true;
                PlayerController->Possess(Character);
                break;
            }
        }
        
        if (bCharacterExist == false)
        {
            ACharacter* Character = SpawnActor<ACharacter>();
            PlayerController->Possess(Character);
            Character->SetActorScale(FVector(0.2f, 0.2f, 0.2f));
        }
        
        APlayerCameraManager* PlayerCameraManager = SpawnActor<APlayerCameraManager>();
        PlayerController->SetPlayerCameraManager(PlayerCameraManager);
    }
}

// AActor* SpawnActorByName(const FString& ActorName, UObject* InOuter, bool bCallBeginPlay)
// {
//     // GetWorld 문제 발생 여지 많음.
//     UClass* ActorClass = UClassRegistry::Get().FindClassByName(ActorName);
//     return GetWorld()->SpawnActorByClass(ActorClass, InOuter, bCallBeginPlay);
// }

/**********************************************************/
