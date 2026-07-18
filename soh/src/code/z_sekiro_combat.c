#include "z_sekiro_combat.h"

#include "overlays/actors/ovl_En_Dekubaba/z_en_dekubaba.h"
#include "overlays/actors/ovl_En_Test/z_en_test.h"
#include "overlays/actors/ovl_En_Zf/z_en_zf.h"
#include "overlays/actors/ovl_En_Wf/z_en_wf.h"
#include "overlays/actors/ovl_En_GeldB/z_en_geldb.h"
#include "overlays/actors/ovl_En_Ik/z_en_ik.h"
#include "overlays/actors/ovl_En_Skb/z_en_skb.h"

s32 Sekiro_UpdateDeathblowFlipTest(PlayState* play, Player* player);

static s32 sDeathblowActive = false;

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

        default:
            return 3;
    }
}

typedef struct {
    s32 active;
    s32 timer;
    Actor* target;
    Vec3f startPos;
    Vec3f endPos;
    f32 previousGravity;
} SekiroFlipTestState;

static SekiroFlipTestState sFlipTest;

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

        case SEKIRO_IMPACT_NONE:
        default:
            break;
    }
}

void Sekiro_StartDeathblowFinisher(PlayState* play, Player* player) {
    Player_SetCsAction(play, NULL, PLAYER_CSACTION_7);
    func_80837948(play, player, PLAYER_MWA_STAB_1H);
}

s32 Sekiro_UpdateDeathblow(PlayState* play, Player* player) {
    Actor* target = player->brokenTarget;
    s32 animFinished;

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
            SEKIRO_IMPACT_RED_BLOOD
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
            SEKIRO_IMPACT_RED_BLOOD
        );
    }

    // Hand off into the genuine native stab
    if (LinkAnimation_OnFrame(&player->skelAnime, 46.0f)) {
        sDeathblowActive = false;
        Sekiro_StartDeathblowFinisher(play, player);
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

void Sekiro_StartDeathblowFlipTest(
    PlayState* play,
    Player* player,
    Actor* target,
    Vec3f* frontPos
) {
    sFlipTest.active = 1;
    sFlipTest.timer = 0;
    sFlipTest.target = target;
    sFlipTest.startPos = player->actor.world.pos;
    sFlipTest.endPos = *frontPos;
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

    Player_SetupAction(
        play,
        player,
        Sekiro_PlayerAction_DeathblowFlip,
        0
    );

    Player_PlaySekiroRoll(player, play);

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

    LinkAnimation_Update(play, &player->skelAnime);

    sFlipTest.timer++;

    t = sFlipTest.timer / 20.0f;
    s16 arcAngle;

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

        Sekiro_StartDeathblowFinisher(play, player);
        return 1;
    }
    return 1;
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
     *     checkFloor
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

s32 Sekiro_TryStartDeathblow(PlayState* play, Player* player) {
    Actor* target = player->brokenTarget;
    Vec3f frontPos;
    Vec3f backPos;
    f32 frontDistSq;
    f32 backDistSq;
    f32 snapDistance = LINK_IS_CHILD ? 50.0f : 70.0f;
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
            Sekiro_StartDeathblowFinisher(play, player);
            return 1;
        }

        /*
        * The route is clear.
        */
        Sekiro_StartDeathblowFlipTest(
            play,
            player,
            target,
            &backPos
        );

        return 1;
    }

    /*
     * BACK CASE:
     *
     * Link is already behind the enemy.
     * Snap him to the exact rear point and start the normal
     * cinematic deathblow.
     */
    player->actor.world.pos.x = backPos.x;
    player->actor.world.pos.z = backPos.z;

    // Face Link directly toward the target.
    yawToTarget =
        Math_Vec3f_Yaw(&player->actor.world.pos, &target->world.pos);

    player->actor.shape.rot.y = yawToTarget;
    player->actor.world.rot.y = yawToTarget;
    player->yaw = yawToTarget;

    // Prevent existing movement from carrying Link away after the snap.
    player->linearVelocity = 0.0f;
    player->actor.speedXZ = 0.0f;
    player->actor.velocity.x = 0.0f;
    player->actor.velocity.y = 0.0f;
    player->actor.velocity.z = 0.0f;

    sDeathblowActive = true;

    Player_SetCsActionWithHaltedActors(
        play,
        &player->actor,
        PLAYER_CSACTION_97
    );

    return 1;
}