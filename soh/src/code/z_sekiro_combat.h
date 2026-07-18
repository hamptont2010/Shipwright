#ifndef Z_SEKIRO_COMBAT_H
#define Z_SEKIRO_COMBAT_H

#include "global.h"

#ifdef __cplusplus
extern "C" {
#endif

u8 Sekiro_GetPostureThreshold(Actor* enemy);
void Sekiro_ApplyPostureBreak(Actor* enemy, PlayState* play);
void Sekiro_PlayDeathblowSwing(void);
s32 Sekiro_UpdateDeathblow(PlayState* play, Player* player);
s32 Sekiro_TryStartDeathblow(PlayState* play, Player* player);
s32 Sekiro_IsDeathblowActive(void);

typedef enum {
    SEKIRO_FINISHER_STAB,
    SEKIRO_FINISHER_SPIN,
    SEKIRO_FINISHER_BACKSLASH_LEFT,
    SEKIRO_FINISHER_BACKSLASH_RIGHT,
    SEKIRO_FINISHER_FLIPSLASH,
    SEKIRO_FINISHER_JUMPSLASH,

    SEKIRO_FINISHER_FORWARD_SLASH,
    SEKIRO_FINISHER_RIGHT_SLASH,
    SEKIRO_FINISHER_LEFT_SLASH,
    SEKIRO_FINISHER_FORWARD_COMBO,
    SEKIRO_FINISHER_RIGHT_COMBO,
    SEKIRO_FINISHER_LEFT_COMBO,
    SEKIRO_FINISHER_STAB_COMBO,
    SEKIRO_FINISHER_BIG_SPIN,
} SekiroDeathblowFinisher;

void Sekiro_StartDeathblowFinisher(
    PlayState* play,
    Player* player,
    SekiroDeathblowFinisher finisher
);

SekiroDeathblowFinisher Sekiro_GetRandomDeathblowFinisher(void);

typedef enum {
    SEKIRO_IMPACT_NONE,
    SEKIRO_IMPACT_RED_BLOOD,
    SEKIRO_IMPACT_GREEN_BLOOD,
    SEKIRO_IMPACT_BLUE_BLOOD,
    SEKIRO_IMPACT_METAL,
} SekiroImpactType;

void Sekiro_SpawnDeathblowImpact(PlayState* play, Actor* target, SekiroImpactType impactType);

void Sekiro_RegisterDeflect(
    Player* player,
    PlayState* play,
    Actor* attacker,
    const Vec3f* deflectPos
);

void Sekiro_LogIkState(
    const char* event,
    s16 armorStatus,
    s16 bodyBreakStatus,
    s16 axeActive,
    s16 deflectTimer
);

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