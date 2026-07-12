#include "z_sekiro_combat.h"

#include "overlays/actors/ovl_En_Dekubaba/z_en_dekubaba.h"
#include "overlays/actors/ovl_En_Test/z_en_test.h"

extern int gMapLoading;

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
) {
    int previousMapLoading = gMapLoading;
    Actor* actor;

    gMapLoading = 0;

    actor = Actor_Spawn(
        actorCtx,
        play,
        actorId,
        posX,
        posY,
        posZ,
        rotX,
        rotY,
        rotZ,
        params
    );

    gMapLoading = previousMapLoading;

    return actor;
}

Actor* Sekiro_SpawnEnemyFromActorEntry(
    ActorContext* actorCtx,
    PlayState* play,
    const ActorEntry* sourceEntry,
    s16 actorId,
    s16 params
) {
    if (sourceEntry == NULL) {
        return NULL;
    }

    return Sekiro_SpawnEnemy(
        actorCtx,
        play,
        actorId,
        sourceEntry->pos.x,
        sourceEntry->pos.y,
        sourceEntry->pos.z,
        0,
        sourceEntry->rot.y,
        0,
        params
    );
}

u8 Sekiro_GetPostureThreshold(Actor* enemy) {
    if (enemy == NULL) {
        return 3;
    }

    switch (enemy->id) {
        case ACTOR_EN_DEKUBABA:
            return 2;

        case ACTOR_EN_TEST:
            return 2;

        default:
            return 3;
    }
}

void Sekiro_ApplyPostureBreak(Actor* enemy) {
    if (enemy == NULL) {
        return;
    }

    switch (enemy->id) {
        case ACTOR_EN_DEKUBABA:
            EnDekubaba_ApplyPostureBreak((EnDekubaba*)enemy);
            break;

        case ACTOR_EN_TEST:
            EnTest_ApplyPostureBreak((EnTest*)enemy);
            break;

        default:
            Actor_SetColorFilter(enemy, 0x4000, 255, 0, 60);
            break;
    }
}