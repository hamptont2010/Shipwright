#include "z_sekiro_combat.h"

#include "overlays/actors/ovl_En_Dekubaba/z_en_dekubaba.h"
#include "overlays/actors/ovl_En_Test/z_en_test.h"
#include "overlays/actors/ovl_En_Zf/z_en_zf.h"
#include "overlays/actors/ovl_En_Wf/z_en_wf.h"
#include "overlays/actors/ovl_En_GeldB/z_en_geldb.h"
#include "overlays/actors/ovl_En_Ik/z_en_ik.h"
#include "overlays/actors/ovl_En_Skb/z_en_skb.h"
#include "overlays/actors/ovl_En_Tite/z_en_tite.h"
#include "overlays/actors/ovl_En_Am/z_en_am.h"
#include "overlays/effects/ovl_Effect_Ss_HitMark/z_eff_ss_hitmark.h"
#include "overlays/actors/ovl_En_Karebaba/z_en_karebaba.h"

s32 Sekiro_UpdateDeathblowFlipTest(PlayState* play, Player* player);

static s32 sDeathblowActive = false;

static SekiroDeathblowFinisher sCinematicFinisher =
    SEKIRO_FINISHER_STAB;

s32 Sekiro_IsDeathblowActive(void) {
    return sDeathblowActive;
}

void Sekiro_PlayerAction_DeathblowFlip(
    Player* player,
    PlayState* play
);

void Player_FinishSekiroFlip(
    Player* player,
    PlayState* play
);

void Player_FinishSekiroFlip(
    Player* this,
    PlayState* play
) {
    Player_SetCsActionWithHaltedActors(
        play,
        &this->actor,
        PLAYER_CSACTION_97
    );
}

void Player_PlaySekiroRoll(Player* player, PlayState* play);

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

s32 Sekiro_IsSupportedEnemy(Actor* enemy) {
    if (enemy == NULL) {
        return false;
    }

    switch (enemy->id) {
        case ACTOR_EN_DEKUBABA:
        case ACTOR_EN_KAREBABA:
        case ACTOR_EN_TEST:
        case ACTOR_EN_ZF:
        case ACTOR_EN_WF:
        case ACTOR_EN_GELDB:
        case ACTOR_EN_IK:
        case ACTOR_EN_SKB:
        case ACTOR_EN_TITE:
        case ACTOR_EN_AM:
            return true;

        default:
            return false;
    }
}

u8 Sekiro_GetPostureThreshold(Actor* enemy) {
    if (enemy == NULL) {
        return 3;
    }

    switch (enemy->id) {
        case ACTOR_EN_DEKUBABA:
            return 1;

        case ACTOR_EN_TEST:
            return 4;

        case ACTOR_EN_ZF:
            return 3;

        case ACTOR_EN_WF:
            return 3;

        case ACTOR_EN_GELDB:
            return 4;

        case ACTOR_EN_IK:
            return 4;

        case ACTOR_EN_SKB:
            return 2;

        case ACTOR_EN_TITE:
            return 3;

        case ACTOR_EN_AM:
            return 3;

        case ACTOR_EN_KAREBABA:
            return 2;

        default:
            return 3;
    }
}

typedef struct {
    s32 active;
    s32 timer;
    s32 rollStarted;
    Actor* target;
    Vec3f startPos;
    Vec3f endPos;
    f32 previousGravity;
    SekiroDeathblowFinisher finisher;
} SekiroFlipTestState;

static SekiroFlipTestState sFlipTest;

typedef struct {
    s32 active;
    s32 timer;
    Actor* target;

    f32 radius;
    f32 startY;

    s16 startYaw;
    s16 yawDelta;

    SekiroDeathblowFinisher finisher;
} SekiroOrbitState;

#define SEKIRO_ORBIT_DURATION 17

static SekiroOrbitState sOrbit;

s32 Sekiro_UpdateDeathblowOrbit(
    PlayState* play,
    Player* player
);

s32 Sekiro_IsDeathblowOrbitPathClear(
    PlayState* play,
    Actor* target,
    Vec3f* startPos,
    f32 radius,
    s16 startYaw,
    s16 yawDelta
) {
    Vec3f previousPos;
    Vec3f nextPos;
    Vec3f hitPos;
    Vec3f floorCheckPos;

    CollisionPoly* hitPoly = NULL;
    CollisionPoly* floorPoly = NULL;

    s32 bgId = BGCHECK_SCENE;
    s32 floorBgId = BGCHECK_SCENE;
    s32 i;

    f32 t;
    f32 previousFloorY;
    f32 floorY;

    s16 orbitYaw;

    previousPos = *startPos;
    previousPos.y += 30.0f;

    floorCheckPos = *startPos;
    floorCheckPos.y += 100.0f;

    previousFloorY = BgCheck_EntityRaycastFloor3(
        &play->colCtx,
        &floorPoly,
        &floorBgId,
        &floorCheckPos
    );

    if (previousFloorY == BGCHECK_Y_MIN) {
        return false;
    }

    /*
     * Break the semicircle into six short wall-test segments.
     */
    for (i = 1; i <= 6; i++) {
        t = i / 6.0f;

        orbitYaw =
            startYaw +
            (s16)(yawDelta * t);

        nextPos.x =
            target->world.pos.x +
            (Math_SinS(orbitYaw) * radius);

        nextPos.y = startPos->y + 30.0f;

        nextPos.z =
            target->world.pos.z +
            (Math_CosS(orbitYaw) * radius);

        floorCheckPos = nextPos;
        floorCheckPos.y = startPos->y + 100.0f;

        floorPoly = NULL;
        floorBgId = BGCHECK_SCENE;

        floorY = BgCheck_EntityRaycastFloor3(
            &play->colCtx,
            &floorPoly,
            &floorBgId,
            &floorCheckPos
        );

        if (floorY == BGCHECK_Y_MIN) {
            return false;
        }

        if (fabsf(floorY - previousFloorY) > 40.0f) {
            return false;
        }

        previousFloorY = floorY;

        hitPoly = NULL;
        bgId = BGCHECK_SCENE;

        if (BgCheck_EntityLineTest1(
                &play->colCtx,
                &previousPos,
                &nextPos,
                &hitPos,
                &hitPoly,
                true,
                true,
                false,
                false,
                &bgId
            )) {
            return false;
        }

        previousPos = nextPos;
    }

    return true;
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

        case ACTOR_EN_SKB:
            EnSkb_ApplyPostureBreak((EnSkb*)enemy);
            break;

        case ACTOR_EN_TITE:
            EnTite_ApplyPostureBreak((EnTite*)enemy);
            break;

        case ACTOR_EN_AM:
            EnAm_ApplyPostureBreak((EnAm*)enemy, play);
            break;

        case ACTOR_EN_KAREBABA:
            EnKarebaba_ApplyPostureBreak((EnKarebaba*)enemy);
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

    if (attacker->category != ACTORCAT_ENEMY) {
        return;
    }

    if (!Sekiro_IsSupportedEnemy(attacker)) {
        return;
    }

    Sekiro_LogDeflect(
        attacker->id,
        attacker->colorFilterTimer,
        player->deflectTimer
    );

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

SekiroImpactType Sekiro_GetDeathblowImpactType(Actor* target) {
    if (target == NULL) {
        return SEKIRO_IMPACT_NONE;
    }

    switch (target->id) {
        case ACTOR_EN_DEKUBABA:
            return SEKIRO_IMPACT_GREEN_BLOOD;

        case ACTOR_EN_ZF:
            return SEKIRO_IMPACT_BLUE_BLOOD;

        case ACTOR_EN_TEST:
            /*
             * Temporary fallback until we identify the exact
             * red dust effect used by native Stalfos impacts.
             */
            return SEKIRO_IMPACT_RED_HITMARK;

        case ACTOR_EN_WF:
            return SEKIRO_IMPACT_RED_BLOOD;

        case ACTOR_EN_GELDB:
            return SEKIRO_IMPACT_RED_BLOOD;

        case ACTOR_EN_IK:
            return SEKIRO_IMPACT_METAL;

        case ACTOR_EN_SKB:
            return SEKIRO_IMPACT_GREEN_BLOOD;

        case ACTOR_EN_TITE:
            return SEKIRO_IMPACT_GREEN_BLOOD;

        case ACTOR_EN_AM:
            return SEKIRO_IMPACT_METAL;

        case ACTOR_EN_KAREBABA:
            return SEKIRO_IMPACT_GREEN_BLOOD;

        default:
            return SEKIRO_IMPACT_RED_BLOOD;
    }
}

void Sekiro_SpawnDeathblowImpact(PlayState* play, Actor* target, SekiroImpactType impactType) {
    Vec3f impactPos;

    if ((target == NULL) || (target->update == NULL)) {
        return;
    }

    impactPos = target->focus.pos;

    switch (impactType) {
        case SEKIRO_IMPACT_RED_BLOOD:
            CollisionCheck_SpawnRedBlood(play, &impactPos);
            break;

        case SEKIRO_IMPACT_GREEN_BLOOD:
            CollisionCheck_GreenBlood(play, NULL, &impactPos);
            break;

        case SEKIRO_IMPACT_BLUE_BLOOD:
            CollisionCheck_BlueBlood(play, NULL, &impactPos);
            break;

        case SEKIRO_IMPACT_METAL:
            CollisionCheck_SpawnShieldParticlesMetalSound(
                play,
                &impactPos,
                &target->projectedPos
            );
            break;

        case SEKIRO_IMPACT_RED_HITMARK:
            EffectSsHitMark_SpawnFixedScale(
                play,
                EFFECT_HITMARK_RED,
                &impactPos
            );
            break;

        case SEKIRO_IMPACT_NONE:
        default:
            break;
    }
}

void Sekiro_StartDeathblowFinisher(
    PlayState* play,
    Player* player,
    SekiroDeathblowFinisher finisher
) {
    s32 meleeAnimation;

    switch (finisher) {
        case SEKIRO_FINISHER_SPIN:
            meleeAnimation = PLAYER_MWA_SPIN_ATTACK_1H;
            break;

        case SEKIRO_FINISHER_BACKSLASH_LEFT:
            meleeAnimation = PLAYER_MWA_BACKSLASH_LEFT;
            break;

        case SEKIRO_FINISHER_BACKSLASH_RIGHT:
            meleeAnimation = PLAYER_MWA_BACKSLASH_RIGHT;
            break;

        case SEKIRO_FINISHER_FLIPSLASH:
            meleeAnimation = PLAYER_MWA_FLIPSLASH_FINISH;
            break;

        case SEKIRO_FINISHER_JUMPSLASH:
            meleeAnimation = PLAYER_MWA_JUMPSLASH_FINISH;
            break;

        case SEKIRO_FINISHER_FORWARD_SLASH:
            meleeAnimation = PLAYER_MWA_FORWARD_SLASH_1H;
            break;

        case SEKIRO_FINISHER_RIGHT_SLASH:
            meleeAnimation = PLAYER_MWA_RIGHT_SLASH_1H;
            break;

        case SEKIRO_FINISHER_LEFT_SLASH:
            meleeAnimation = PLAYER_MWA_LEFT_SLASH_1H;
            break;

        case SEKIRO_FINISHER_FORWARD_COMBO:
            meleeAnimation = PLAYER_MWA_FORWARD_COMBO_1H;
            break;

        case SEKIRO_FINISHER_RIGHT_COMBO:
            meleeAnimation = PLAYER_MWA_RIGHT_COMBO_1H;
            break;

        case SEKIRO_FINISHER_LEFT_COMBO:
            meleeAnimation = PLAYER_MWA_LEFT_COMBO_1H;
            break;

        case SEKIRO_FINISHER_STAB_COMBO:
            meleeAnimation = PLAYER_MWA_STAB_COMBO_1H;
            break;

        case SEKIRO_FINISHER_BIG_SPIN:
            meleeAnimation = PLAYER_MWA_BIG_SPIN_1H;
            break;

        case SEKIRO_FINISHER_STAB:
        default:
            meleeAnimation = PLAYER_MWA_STAB_1H;
            break;
    }

    Player_SetCsAction(play, NULL, PLAYER_CSACTION_7);
    func_80837948(play, player, meleeAnimation);
}

SekiroDeathblowFinisher Sekiro_GetRandomDeathblowFinisher(void) {
    static const SekiroDeathblowFinisher sFinishers[] = {
        SEKIRO_FINISHER_STAB,
        SEKIRO_FINISHER_FLIPSLASH,
        SEKIRO_FINISHER_FORWARD_SLASH,
        SEKIRO_FINISHER_RIGHT_SLASH,
        SEKIRO_FINISHER_LEFT_SLASH,
        SEKIRO_FINISHER_RIGHT_COMBO,
        SEKIRO_FINISHER_LEFT_COMBO,
        SEKIRO_FINISHER_STAB_COMBO,
    };

    s32 finisherCount =
        sizeof(sFinishers) / sizeof(sFinishers[0]);

    s32 index = (s32)Rand_ZeroFloat((f32)finisherCount);

    if (index >= finisherCount) {
        index = finisherCount - 1;
    }

    return sFinishers[index];
}

s32 Sekiro_UpdateDeathblow(PlayState* play, Player* player) {
    Actor* target = player->brokenTarget;
    SekiroImpactType impactType;
    s32 animFinished;

    impactType = Sekiro_GetDeathblowImpactType(target);

    player->skelAnime.playSpeed = 1.5f;

    animFinished = LinkAnimation_Update(play, &player->skelAnime);

    // First cinematic slash
    if (LinkAnimation_OnFrame(&player->skelAnime, 10.0f)) {
        Sekiro_PlayDeathblowSwing();
    }

    // First impact
    if (LinkAnimation_OnFrame(&player->skelAnime, 14.0f)) {
        Sekiro_SpawnDeathblowImpact(
            play,
            target,
            impactType
        );
    }

    // Second cinematic slash
    if (LinkAnimation_OnFrame(&player->skelAnime, 30.0f)) {
        Sekiro_PlayDeathblowSwing();
    }

    // Second impact
    if (LinkAnimation_OnFrame(&player->skelAnime, 34.0f)) {
        Sekiro_SpawnDeathblowImpact(
            play,
            target,
            impactType
        );
    }

    // Hand off into the genuine native stab
    if (LinkAnimation_OnFrame(&player->skelAnime, 46.0f)) {
        sDeathblowActive = false;

        Sekiro_StartDeathblowFinisher(
            play,
            player,
            sCinematicFinisher
        );
    
        return true;
    }

    // Safety fallback if the cinematic reaches its natural end
    if (animFinished) {
        sDeathblowActive = false;
        Player_SetCsAction(play, NULL, PLAYER_CSACTION_7);
        return true;
    }

    return false;
}

void Sekiro_PlayerAction_DeathblowFlip(Player* player, PlayState* play) {
    Sekiro_UpdateDeathblowFlipTest(play, player);
}

void Sekiro_PlayerAction_DeathblowOrbit(
    Player* player,
    PlayState* play
) {
    Sekiro_UpdateDeathblowOrbit(play, player);
}

s32 Sekiro_StartDeathblowOrbit(
    PlayState* play,
    Player* player,
    Actor* target
) {
    f32 dx;
    f32 dz;
    s32 clockwiseClear;
    s32 counterClockwiseClear;

    sOrbit.active = true;
    sOrbit.timer = 0;
    sOrbit.target = target;
    sOrbit.startY = player->actor.world.pos.y;
    sOrbit.finisher = Sekiro_GetRandomDeathblowFinisher();

    /*
     * Determine Link's current angle around the enemy.
     */
    sOrbit.startYaw = Math_Vec3f_Yaw(
        &target->world.pos,
        &player->actor.world.pos
    );

    /*
     * Current radius from the enemy.
     */
    dx = player->actor.world.pos.x - target->world.pos.x;
    dz = player->actor.world.pos.z - target->world.pos.z;

    sOrbit.radius = sqrtf(SQ(dx) + SQ(dz));

    /*
     * For the first test, always travel clockwise by 180 degrees.
     */
    clockwiseClear = Sekiro_IsDeathblowOrbitPathClear(
        play,
        target,
        &player->actor.world.pos,
        sOrbit.radius,
        sOrbit.startYaw,
        0x8000
    );

    counterClockwiseClear = Sekiro_IsDeathblowOrbitPathClear(
        play,
        target,
        &player->actor.world.pos,
        sOrbit.radius,
        sOrbit.startYaw,
        -0x8000
    );

    if (clockwiseClear && counterClockwiseClear) {
        sOrbit.yawDelta =
            (Rand_ZeroOne() < 0.5f)
                ? 0x8000
                : -0x8000;
    } else if (clockwiseClear) {
        sOrbit.yawDelta = 0x8000;
    } else if (counterClockwiseClear) {
        sOrbit.yawDelta = -0x8000;
    } else {
        sOrbit.active = false;

        osSyncPrintf(
            "SEKIRO: orbit blocked both directions\n"
        );

        return 0;
    }

    player->linearVelocity = 0.0f;
    player->actor.speedXZ = 0.0f;
    player->actor.velocity.x = 0.0f;
    player->actor.velocity.y = 0.0f;
    player->actor.velocity.z = 0.0f;

    Player_SetupAction(
        play,
        player,
        Sekiro_PlayerAction_DeathblowOrbit,
        0
    );

    Player_PlaySekiroRoll(player, play);

    osSyncPrintf(
        "SEKIRO: orbit selected finisher=%d\n",
        sOrbit.finisher
    );

    return 1;
}

void Sekiro_StartDeathblowFlipTest(
    PlayState* play,
    Player* player,
    Actor* target,
    Vec3f* destination
) {
    sFlipTest.active = 1;
    sFlipTest.timer = 0;
    sFlipTest.rollStarted = false;
    sFlipTest.target = target;
    sFlipTest.startPos = player->actor.world.pos;
    sFlipTest.endPos = *destination;
    sFlipTest.endPos.y = player->actor.world.pos.y;
    sFlipTest.previousGravity = player->actor.gravity;

    player->linearVelocity = 0.0f;
    player->actor.speedXZ = 0.0f;
    player->actor.velocity.x = 0.0f;
    player->actor.velocity.y = 0.0f;
    player->actor.velocity.z = 0.0f;
    player->actor.gravity = 0.0f;
    player->actor.bgCheckFlags &= ~1;
    player->stateFlags3 |= PLAYER_STATE3_MIDAIR;

    sFlipTest.finisher = Sekiro_GetRandomDeathblowFinisher();

    osSyncPrintf(
        "SEKIRO: flip selected finisher=%d\n",
        (s32)sFlipTest.finisher
    );

    Player_SetupAction(
        play,
        player,
        Sekiro_PlayerAction_DeathblowFlip,
        0
    );

}

s32 Sekiro_UpdateDeathblowFlipTest(
    PlayState* play,
    Player* player
) {
    f32 t;
    f32 height;

    if (!sFlipTest.active) {
        return 0;
    }

    if ((sFlipTest.target == NULL) ||
        (sFlipTest.target->update == NULL)) {
        player->actor.gravity = sFlipTest.previousGravity;
        player->actor.velocity.y = 0.0f;
        player->stateFlags3 &= ~PLAYER_STATE3_MIDAIR;

        sFlipTest.active = 0;
        Player_SetCsAction(play, NULL, PLAYER_CSACTION_7);
        return 0;
    }

    sFlipTest.timer++;

    if (!sFlipTest.rollStarted &&
        (sFlipTest.timer >= 2)) {
        Player_PlaySekiroRoll(player, play);
        sFlipTest.rollStarted = true;
    }

    if (sFlipTest.rollStarted) {
        LinkAnimation_Update(play, &player->skelAnime);
    }

    t = sFlipTest.timer / 20.0f;

    if (t > 1.0f) {
        t = 1.0f;
    }

    player->actor.gravity = 0.0f;
    player->actor.velocity.y = 0.0f;
    player->actor.bgCheckFlags &= ~1;
    player->stateFlags3 |= PLAYER_STATE3_MIDAIR;

    player->actor.world.pos.x =
        sFlipTest.startPos.x +
        ((sFlipTest.endPos.x - sFlipTest.startPos.x) * t);

    player->actor.world.pos.z =
        sFlipTest.startPos.z +
        ((sFlipTest.endPos.z - sFlipTest.startPos.z) * t);

    height = Math_SinS((s16)(t * 0x8000)) * 80.0f;
    
    player->actor.world.pos.y =
        sFlipTest.startPos.y + height;

    player->actor.shape.rot.y =
        Math_Vec3f_Yaw(
            &player->actor.world.pos,
            &sFlipTest.target->world.pos
        );

    player->actor.world.rot.y = player->actor.shape.rot.y;
    player->yaw = player->actor.shape.rot.y;

    if (sFlipTest.timer >= 20) {
        s16 yawToTarget;

        player->actor.world.pos = sFlipTest.endPos;

        player->actor.gravity = sFlipTest.previousGravity;
        player->actor.velocity.y = 0.0f;
        player->stateFlags3 &= ~PLAYER_STATE3_MIDAIR;

        yawToTarget = Math_Vec3f_Yaw(
            &player->actor.world.pos,
            &sFlipTest.target->world.pos
        );

        player->actor.shape.rot.y = yawToTarget;
        player->actor.world.rot.y = yawToTarget;
        player->yaw = yawToTarget;

        sFlipTest.active = 0;

        Sekiro_StartDeathblowFinisher(
            play,
            player,
            sFlipTest.finisher
        );
        return 1;
    }
    return 1;
}

s32 Sekiro_UpdateDeathblowOrbit(
    PlayState* play,
    Player* player
) {
    f32 t;
    s16 orbitYaw;
    s16 yawToTarget;

    if (!sOrbit.active) {
        return 0;
    }

    if ((sOrbit.target == NULL) ||
        (sOrbit.target->update == NULL)) {
        sOrbit.active = false;
        Player_SetCsAction(play, NULL, PLAYER_CSACTION_7);
        return 0;
    }

    LinkAnimation_Update(
        play,
        &player->skelAnime
    );

    sOrbit.timer++;

    /*
     * Start with the same 20-frame duration as the aerial flip.
     */
    t = sOrbit.timer / (f32)SEKIRO_ORBIT_DURATION;

    if (t > 1.0f) {
        t = 1.0f;
    }

    orbitYaw =
        sOrbit.startYaw +
        (s16)(sOrbit.yawDelta * t);

    /*
     * Move Link around the enemy rather than directly through it.
     */
    player->actor.world.pos.x =
        sOrbit.target->world.pos.x +
        (Math_SinS(orbitYaw) * sOrbit.radius);

    player->actor.world.pos.z =
        sOrbit.target->world.pos.z +
        (Math_CosS(orbitYaw) * sOrbit.radius);

    /*
     * Flat-ground test only.
     */
    player->actor.world.pos.y = sOrbit.startY;

    /*
    * Keep Link facing the enemy while he rolls around it.
    */
    yawToTarget = Math_Vec3f_Yaw(
        &player->actor.world.pos,
        &sOrbit.target->world.pos
    );

    player->actor.shape.rot.y = yawToTarget;
    player->actor.world.rot.y = yawToTarget;
    player->yaw = yawToTarget;

    player->linearVelocity = 0.0f;
    player->actor.speedXZ = 0.0f;
    player->actor.velocity.x = 0.0f;
    player->actor.velocity.y = 0.0f;
    player->actor.velocity.z = 0.0f;

    if (sOrbit.timer >= SEKIRO_ORBIT_DURATION) {
        /*
         * Face the enemy immediately before launching the real attack.
         */
        yawToTarget = Math_Vec3f_Yaw(
            &player->actor.world.pos,
            &sOrbit.target->world.pos
        );

        player->actor.shape.rot.y = yawToTarget;
        player->actor.world.rot.y = yawToTarget;
        player->yaw = yawToTarget;

        sOrbit.active = false;

        Sekiro_StartDeathblowFinisher(
            play,
            player,
            sOrbit.finisher
        );

        return 1;
    }

    return 1;
}

s32 Sekiro_IsDeathblowLandingSafe(
    PlayState* play,
    Vec3f* startPos,
    Vec3f* landingPos
) {
    Vec3f startCheckPos;
    Vec3f landingCheckPos;
    CollisionPoly* floorPoly = NULL;
    s32 bgId = BGCHECK_SCENE;
    f32 startFloorY;
    f32 landingFloorY;

    /*
     * Start the raycasts above the expected floor.
     */
    startCheckPos = *startPos;
    startCheckPos.y += 100.0f;

    landingCheckPos = *landingPos;
    landingCheckPos.y += 100.0f;

    /*
     * Find the floor beneath the launch point.
     */
    startFloorY = BgCheck_EntityRaycastFloor3(
        &play->colCtx,
        &floorPoly,
        &bgId,
        &startCheckPos
    );

    if (startFloorY == BGCHECK_Y_MIN) {
        return false;
    }

    /*
     * Reset the output variables before checking the landing point.
     */
    floorPoly = NULL;
    bgId = BGCHECK_SCENE;

    /*
     * Find the floor beneath the landing point.
     */
    landingFloorY = BgCheck_EntityRaycastFloor3(
        &play->colCtx,
        &floorPoly,
        &bgId,
        &landingCheckPos
    );

    if (landingFloorY == BGCHECK_Y_MIN) {
        return false;
    }

    /*
     * Reject a landing that is much higher or lower than the launch.
     */
    if (fabsf(landingFloorY - startFloorY) > 40.0f) {
        return false;
    }

    return true;
}

s32 Sekiro_IsDeathblowFlipPathClear(
    PlayState* play,
    Vec3f* startPos,
    Vec3f* endPos
) {
    Vec3f lineStart = *startPos;
    Vec3f lineEnd = *endPos;
    Vec3f hitPos;
    CollisionPoly* hitPoly = NULL;
    s32 bgId = BGCHECK_SCENE;

    /*
     * Raise the test above the floor.
     *
     * Testing directly at Link's feet risks the line touching floor
     * geometry instead of detecting only an obstructing wall.
     */
    lineStart.y += 30.0f;
    lineEnd.y += 30.0f;

    /*
     * Check the entire horizontal route from the launch point to the
     * landing point.
     *
     * Parameters after hitPoly:
     *     checkOneFace
     *     checkWall
     *     checkFloors
     *     checkCeiling
     *
     * We care about walls here, not floors or ceilings.
     */
    if (BgCheck_EntityLineTest1(
            &play->colCtx,
            &lineStart,
            &lineEnd,
            &hitPos,
            &hitPoly,
            true,
            true,
            false,
            false,
            &bgId
        )) {
        return false;
    }

    return true;
}

void Sekiro_StartDeathblowCinematic(
    PlayState* play,
    Player* player
) {
    sDeathblowActive = true;
    sCinematicFinisher = Sekiro_GetRandomDeathblowFinisher();

    osSyncPrintf(
        "SEKIRO: cinematic selected finisher=%d\n",
        (s32)sCinematicFinisher
    );

    Player_SetCsActionWithHaltedActors(
        play,
        &player->actor,
        PLAYER_CSACTION_97
    );
}

s32 Sekiro_TryStartDeathblow(PlayState* play, Player* player) {
    Actor* target = player->brokenTarget;
    Vec3f frontPos;
    Vec3f backPos;
    f32 frontDistSq;
    f32 backDistSq;
    f32 snapDistance = LINK_IS_CHILD ? 45.0f : 65.0f;
    f32 sinYaw;
    f32 cosYaw;
    s16 yawToTarget;
    s32 flipPathClear;

    // There is no valid posture-broken target.
    if ((target == NULL) || (target->update == NULL)) {
        sDeathblowActive = false;
        Player_SetCsAction(play, NULL, PLAYER_CSACTION_7);
        return 0;
    }

    // The broken enemy must be Link's current hostile lock-on target.
    if ((player->focusActor != target) ||
        !Player_CheckHostileLockOn(player)) {
        return 0;
    }

    /*
     * Calculate points directly in front of and behind the enemy,
     * based on the direction the enemy is facing.
     */
    sinYaw = Math_SinS(target->shape.rot.y);
    cosYaw = Math_CosS(target->shape.rot.y);

    frontPos = target->world.pos;
    frontPos.x += sinYaw * snapDistance;
    frontPos.z += cosYaw * snapDistance;

    backPos = target->world.pos;
    backPos.x -= sinYaw * snapDistance;
    backPos.z -= cosYaw * snapDistance;

    /*
     * Determine whether Link is currently closer to the enemy's
     * front or back.
     */
    frontDistSq =
        SQ(player->actor.world.pos.x - frontPos.x) +
        SQ(player->actor.world.pos.z - frontPos.z);

    backDistSq =
        SQ(player->actor.world.pos.x - backPos.x) +
        SQ(player->actor.world.pos.z - backPos.z);

    /*
     * FRONT CASE:
     *
     * Link is closer to the enemy's front.
     * Snap him to the exact launch point first, then flip to the
     * exact point behind the enemy.
     */
    if (frontDistSq <= backDistSq) {
        /*
        * Make sure there is no wall between the flip's launch and
        * landing positions.
        */
        flipPathClear = Sekiro_IsDeathblowFlipPathClear(
            play,
            &frontPos,
            &backPos
        );

        if (!Sekiro_IsDeathblowLandingSafe(
                play,
                &frontPos,
                &backPos
            )) {
            flipPathClear = false;
        }

        /*
        * Snap Link to the front launch point.
        */
        player->actor.world.pos.x = frontPos.x;
        player->actor.world.pos.z = frontPos.z;

        /*
        * Face Link toward the enemy.
        */
        yawToTarget =
            Math_Vec3f_Yaw(&player->actor.world.pos, &target->world.pos);

        player->actor.shape.rot.y = yawToTarget;
        player->actor.world.rot.y = yawToTarget;
        player->yaw = yawToTarget;

        /*
        * Remove normal movement before either the flip or fallback stab.
        */
        player->linearVelocity = 0.0f;
        player->actor.speedXZ = 0.0f;
        player->actor.velocity.x = 0.0f;
        player->actor.velocity.y = 0.0f;
        player->actor.velocity.z = 0.0f;

        /*
        * A wall blocks the route behind the enemy.
        * Skip the flip and stab from the front.
        */
        if (!flipPathClear) {
            Sekiro_StartDeathblowCinematic(play, player);
            return 1;
        }

        /*
        * The route is clear.
        */
        if (Rand_ZeroOne() < 0.5f) {
            Sekiro_StartDeathblowFlipTest(
                play,
                player,
                target,
                &backPos
            );
        } else if (!Sekiro_StartDeathblowOrbit(
                    play,
                    player,
                    target
                )) {
            Sekiro_StartDeathblowFlipTest(
                play,
                player,
                target,
                &backPos
            );
        }

        return 1;
    }

    /*
    * BACK CASE:
    *
    * Link is already behind the enemy.
    * Snap him to the exact rear point.
    *
    * From here, randomly choose between:
    *     1. The Ganon cinematic from behind.
    *     2. A reverse aerial flip to the front.
    */

    player->actor.world.pos.x = backPos.x;
    player->actor.world.pos.z = backPos.z;

    /*
    * Face Link directly toward the target before either entry begins.
    */
    yawToTarget =
        Math_Vec3f_Yaw(
            &player->actor.world.pos,
            &target->world.pos
        );

    player->actor.shape.rot.y = yawToTarget;
    player->actor.world.rot.y = yawToTarget;
    player->yaw = yawToTarget;

    /*
    * Prevent existing movement from carrying Link away after the snap.
    */
    player->linearVelocity = 0.0f;
    player->actor.speedXZ = 0.0f;
    player->actor.velocity.x = 0.0f;
    player->actor.velocity.y = 0.0f;
    player->actor.velocity.z = 0.0f;

    /*
    * Check whether the reverse flip route from behind the enemy
    * to the front anchor is clear.
    */
    flipPathClear = Sekiro_IsDeathblowFlipPathClear(
        play,
        &backPos,
        &frontPos
    );

    if (!Sekiro_IsDeathblowLandingSafe(
            play,
            &backPos,
            &frontPos
        )) {
        flipPathClear = false;
    }

    /*
    * If the route is clear, randomly choose between the cinematic
    * and the reverse flip.
    *
    * If the route is blocked, always use the cinematic.
    */
    if (flipPathClear && (Rand_ZeroOne() < 0.5f)) {
        osSyncPrintf(
            "SEKIRO: rear entry=reverse flip\n"
        );

        Sekiro_StartDeathblowFlipTest(
            play,
            player,
            target,
            &frontPos
        );

        return 1;
    }

    osSyncPrintf(
        "SEKIRO: rear entry=cinematic\n"
    );

    Sekiro_StartDeathblowCinematic(
        play,
        player
    );

    return 1;
}