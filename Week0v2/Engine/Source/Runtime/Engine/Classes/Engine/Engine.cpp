#include "Engine.h"
#include "ThirdParty/PhysX/include/PxPhysicsAPI.h"
void UEngine::Init()
{
    static PxDefaultAllocator      GAllocator;
    static PxDefaultErrorCallback  GErrorCallback;

    // 1. PxFoundation 생성
    PvdFoundation = PxCreateFoundation(PX_PHYSICS_VERSION, GAllocator, GErrorCallback);
    if (!PvdFoundation)
    {
        printf("Fail: PhysX Foundation\n");
        return;
    }

    // 2. PVD 설정 및 연결
    Pvd = PxCreatePvd(*PvdFoundation);
    PvdTransport = PxDefaultPvdSocketTransportCreate("127.0.0.1", 5425, 10); // IP, Port, timeout
    if (Pvd && PvdTransport)
    {
        Pvd->connect(*PvdTransport, PxPvdInstrumentationFlag::eALL);
    }

    // 3. Physics 생성
    PxTolerancesScale scale;
    Physics = PxCreatePhysics(PX_PHYSICS_VERSION, *PvdFoundation, scale, true, Pvd);
    if (!Physics)
    {
        printf("Fail: PhysX Physics\n");
        return;
    }

    printf("Init : PhysX + PVD \n");
}

void UEngine::Tick(float DeltaTime)
{
    
}

void UEngine::Release()
{
    if (Physics)
    {
        Physics->release();
        Physics = nullptr;
    }

    if (Pvd)
    {
        Pvd->release();
        Pvd = nullptr;
    }

    if (PvdTransport)
    {
        PvdTransport->release();
        PvdTransport = nullptr;
    }

    if (PvdFoundation)
    {
        PvdFoundation->release();
        PvdFoundation = nullptr;
    }

    printf("PhysX + PVD 해제 완료\n");
}
