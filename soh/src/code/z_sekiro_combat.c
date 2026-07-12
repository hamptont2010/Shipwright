#include "z_sekiro_combat.h"

#include "overlays/actors/ovl_En_Dekubaba/z_en_dekubaba.h"

u8 Sekiro_GetPostureThreshold(Actor* enemy) {
    if (enemy == NULL) {
        return 3;
    }

    switch (enemy->id) {
        case ACTOR_EN_DEKUBABA:
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

        default:
            Actor_SetColorFilter(enemy, 0x4000, 255, 0, 60);
            break;
    }
}