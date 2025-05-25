#include "PhysXCallback.h"
#include <cstdio>
using namespace physx;

void MySimulationEventCallback::onContact(const PxContactPairHeader& pairHeader,
                                          const PxContactPair* pairs,
                                          PxU32 nbPairs)
{
    printf("[Contact] between %p and %p\n", pairHeader.actors[0], pairHeader.actors[1]);
    PxRigidActor* actorA = pairHeader.actors[0];
    PxRigidActor* actorB = pairHeader.actors[1];

    void* dataA = actorA->userData;
    void* dataB = actorB->userData;

    for (PxU32 i = 0; i < nbPairs; ++i)
    {
        const PxContactPair& cp = pairs[i];
        if(cp.events & PxPairFlag::eNOTIFY_TOUCH_FOUND) {
            // 접촉 시작!
        }
        if(cp.events & PxPairFlag::eNOTIFY_TOUCH_PERSISTS) {
            // 접촉 유지(매 프레임)
        }
        if(cp.events & PxPairFlag::eNOTIFY_TOUCH_LOST) {
            // 접촉 해제!
        }
    }
}

void MySimulationEventCallback::onTrigger(PxTriggerPair*, PxU32)
{
}

void MySimulationEventCallback::onConstraintBreak(PxConstraintInfo*, PxU32)
{
}

void MySimulationEventCallback::onWake(PxActor**, PxU32)
{
}

void MySimulationEventCallback::onSleep(PxActor**, PxU32)
{
}

void MySimulationEventCallback::onAdvance(const PxRigidBody* const*,
                                          const PxTransform*,
                                          const PxU32)
{
}













