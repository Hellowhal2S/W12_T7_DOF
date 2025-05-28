#include "PhysXCallback.h"
#include <cstdio>

#include "UserInterface/Console.h"
using namespace physx;

void MySimulationEventCallback::onContact(const PxContactPairHeader& pairHeader,
                                          const PxContactPair* pairs,
                                          PxU32 nbPairs)
{
    // 삭제 플래그 체크
    bool removedActor0 = (pairHeader.flags & PxContactPairHeaderFlag::eREMOVED_ACTOR_0);
    bool removedActor1 = (pairHeader.flags & PxContactPairHeaderFlag::eREMOVED_ACTOR_1);

    if (removedActor0 || removedActor1)
    {
        // 이미 삭제된 액터가 포함된 이벤트는 무시하거나 별도 처리
        return;
    }
    
    PxRigidActor* actorA = pairHeader.actors[0];
    PxRigidActor* actorB = pairHeader.actors[1];

    const char*  nameA   = actorA->getName();
    const char*  nameB   = actorB->getName();

    for (PxU32 i = 0; i < nbPairs; ++i)
    {
        const PxContactPair& cp = pairs[i];
        // 접촉 시작!
        if(cp.events & PxPairFlag::eNOTIFY_TOUCH_FOUND)
        {
            UE_LOG(LogLevel::Display, "[Contact Start] %s, %s\n", nameA, nameB);
        }
        // 접촉 유지(매 프레임)
        if(cp.events & PxPairFlag::eNOTIFY_TOUCH_PERSISTS)
        {
            UE_LOG(LogLevel::Display, "[Contact Persist] %s, %s\n", nameA, nameB);
        }
        // 접촉 해제!
        if(cp.events & PxPairFlag::eNOTIFY_TOUCH_LOST)
        {
            UE_LOG(LogLevel::Display, "[Contact End] %s, %s\n", nameA, nameB);
        }
    }
}

void MySimulationEventCallback::onTrigger(PxTriggerPair* pairs, PxU32 count)
{
    for (PxU32 i = 0; i < count; ++i)
    {
        const PxTriggerPair& tp = pairs[i];
        PxRigidActor* trgActor = tp.triggerActor;  // 트리거 박스 액터
        PxRigidActor* othActor = tp.otherActor;    // 겹친 다른 액터
        const char* triggerName = trgActor->getName();
        const char* otherName   = othActor->getName();

        // Enter(겹침 시작)
        if (tp.status & PxPairFlag::eNOTIFY_TOUCH_FOUND)
            UE_LOG(LogLevel::Display, "[Trigger Enter] %s <= %s\n", triggerName, otherName);

        // Leave(겹침 해제)
        if (tp.status & PxPairFlag::eNOTIFY_TOUCH_LOST)
            UE_LOG(LogLevel::Display, "[Trigger Leave] %s => %s\n", triggerName, otherName);
    }
}

void MySimulationEventCallback::onConstraintBreak(PxConstraintInfo* constraints, PxU32 count)
{
    for (PxU32 i = 0; i < count; ++i)
    {
        const PxConstraintInfo& info = constraints[i];
        // 깨진 Constraint 포인터와 외부 레퍼런스, 타입을 출력
        UE_LOG(LogLevel::Display, "[Constraint Break] constraint=%p, externalRef=%p, type=%u\n",
               info.constraint,
               info.externalReference,
               (unsigned)info.type);
    }
}

void MySimulationEventCallback::onWake(PxActor** actors, PxU32 count)
{
    for (PxU32 i = 0; i < count; ++i)
    {
        PxActor* actor = actors[i];
        UE_LOG(LogLevel::Display, "[Wake] Actor = %s\n", actor->getName());
    }
}

void MySimulationEventCallback::onSleep(PxActor** actors, PxU32 count)
{
    for (PxU32 i = 0; i < count; ++i)
    {
        PxActor* actor = actors[i];
        UE_LOG(LogLevel::Display, "[Sleep] Actor = %s\n", actor->getName());
    }
}

void MySimulationEventCallback::onAdvance(
    const PxRigidBody* const* bodyBuffer,
    const PxTransform*        poseBuffer,
    const PxU32               count)
{
    for (PxU32 i = 0; i < count; ++i)
    {
        const PxRigidBody* body = bodyBuffer[i];
        const char*          name = body->getName();            // 액터 이름
        const PxVec3&        pos  = poseBuffer[i].p;            // 새 위치

        UE_LOG(LogLevel::Display, "[Advance] %s NewPos=(%.3f, %.3f, %.3f)\n",
               name,                                         // 이름
               pos.x, pos.y, pos.z);                        // 좌표
    }
}