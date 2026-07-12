#ifndef Z_SEKIRO_COMBAT_H
#define Z_SEKIRO_COMBAT_H

#include "global.h"

#ifdef __cplusplus
extern "C" {
#endif

u8 Sekiro_GetPostureThreshold(Actor* enemy);
void Sekiro_ApplyPostureBreak(Actor* enemy);

Actor* Sekiro_SpawnEnemy(
    ActorContext* actorCtx,
    PlayState* play,
    s16 actorId,
    f32 posX,
    f32 posY,
    f32 posZ,
    s16 rotX,
    s16 rotY,
    s16 rotZ,
    s16 params
);

Actor* Sekiro_SpawnEnemyFromActorEntry(
    ActorContext* actorCtx,
    PlayState* play,
    const ActorEntry* sourceEntry,
    s16 actorId,
    s16 params
);

#ifdef __cplusplus
}
#endif

#endif // Z_SEKIRO_COMBAT_H