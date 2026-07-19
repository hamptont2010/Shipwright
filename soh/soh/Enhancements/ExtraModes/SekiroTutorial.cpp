#include "SekiroTutorial.h"

#include "soh/ShipInit.hpp"
#include "soh/Enhancements/game-interactor/GameInteractor.h"
#include "soh/Enhancements/custom-message/CustomMessageManager.h"

#include <spdlog/spdlog.h>

void BuildSekiroTutorialMessage(uint16_t* textId, bool* loadFromMessageTable) {

    SPDLOG_INFO("Sekiro tutorial callback");

    CustomMessage msg(
        "%rSekiro Combat%w^"
        "You must be locked onto an enemy to Deflect!^"
        "Press %rR%w just before an enemy's strike lands to Deflect.^"
        "Break an enemy's Posture with enough Deflects.^"
        "When the targeting arrows turn %rred%w, press %rA%w to perform a Deathblow."
    );

    msg.AutoFormat();
    msg.LoadIntoFont();

    *loadFromMessageTable = false;
}

void RegisterSekiroTutorialMessages() {

    SPDLOG_INFO("RegisterSekiroTutorialMessages");

    COND_ID_HOOK(
        OnOpenText,
        0x031F,
        true,
        BuildSekiroTutorialMessage);
}
