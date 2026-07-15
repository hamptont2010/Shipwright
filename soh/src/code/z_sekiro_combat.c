#include "z_sekiro_combat.h"

#include "overlays/actors/ovl_En_Dekubaba/z_en_dekubaba.h"
#include "overlays/actors/ovl_En_Test/z_en_test.h"
#include "overlays/actors/ovl_En_Zf/z_en_zf.h"
#include "overlays/actors/ovl_En_Wf/z_en_wf.h"
#include "overlays/actors/ovl_En_GeldB/z_en_geldb.h"
#include "overlays/actors/ovl_En_Ik/z_en_ik.h"

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

        case ACTOR_EN_ZF:
            return 3;

        case ACTOR_EN_WF:
            return 3;

        case ACTOR_EN_GELDB:
            return 2;

        case ACTOR_EN_IK:
            return 2;

        default:
            return 3;
    }
}

void Sekiro_LogDeflect(
    s16 actorId,
    s16 colorTimer,
    u8 deflectTimer
);

void Sekiro_ApplyPostureBreak(Actor* enemy, PlayState* play) {
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

        case ACTOR_EN_ZF:
            EnZf_ApplyPostureBreak((EnZf*)enemy);
            break;

        case ACTOR_EN_WF:
            EnWf_ApplyPostureBreak((EnWf*)enemy);
            break;
            
        case ACTOR_EN_GELDB:
            EnGeldB_ApplyPostureBreak((EnGeldB*)enemy);
            break;

        case ACTOR_EN_IK:
            EnIk_ApplyPostureBreak((EnIk*)enemy, play);
            break;

        default:
            Actor_SetColorFilter(enemy, 0x4000, 255, 0, 60);
            break;
    }
}


void Sekiro_RegisterDeflect(
    Player* player,
    PlayState* play,
    Actor* attacker,
    const Vec3f* deflectPos
) {
    u8 postureThreshold;

    if ((player == NULL) || (play == NULL) || (attacker == NULL)) {
        return;
    }

    Sekiro_LogDeflect(
        attacker->id,
        attacker->colorFilterTimer,
        player->deflectTimer
    );

    if (attacker->category != ACTORCAT_ENEMY) {
        return;
    }

    if (attacker->freezeTimer != 0) {
        return;
    }

    player->linearVelocity = 0.0f;

    if (deflectPos != NULL) {
        CollisionCheck_SpawnShieldParticlesMetal(play, deflectPos);
        CollisionCheck_SpawnShieldParticlesMetal(play, deflectPos);
    }

    Player_PlaySfx(player, NA_SE_IT_SHIELD_REFLECT_SW);

    postureThreshold = Sekiro_GetPostureThreshold(attacker);

    if (player->deflectTarget == attacker) {
        player->deflectCount++;
    } else {
        player->deflectTarget = attacker;
        player->deflectCount = 1;
    }

    osSyncPrintf(
        "SEKIRO: attacker=%d posture=%d/%d\n",
        attacker->id,
        player->deflectCount,
        postureThreshold
    );

    if (player->deflectCount >= postureThreshold) {
        player->brokenTarget = attacker;
        player->brokenTimer = 60;

        osSyncPrintf(
            "SEKIRO: posture break attacker=%d\n",
            attacker->id
        );

        Sekiro_ApplyPostureBreak(attacker, play);

        player->deflectTarget = NULL;
        player->deflectCount = 0;
    } else {
        Actor_SetColorFilter(attacker, 0, 255, 0, 20);
    }
}

/**
 * Deathblow logic
 */

 void Sekiro_PlayDeathblowSwing(void) {
    Sfx_PlaySfxCentered(NA_SE_IT_SWORD_SWING_HARD);
    Sfx_PlaySfxCentered(NA_SE_VO_LI_SWORD_N);
}