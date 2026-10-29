#include "mods/service.hpp"
#include "mods/svc/hook.hpp"
#include "mods/svc/log.hpp"
#include "mods/svc/game_mode.h"

#include "f_op/f_op_actor_mng.h"
#include "d/d_com_inf_game.h"
#include "d/actor/d_a_alink.h"
#include "d/d_pane_class.h"
#include "d/d_meter2_draw.h"

DEFINE_MOD();

IMPORT_SERVICE(LogService, svc_log);
IMPORT_SERVICE(HookService, svc_hook);
IMPORT_SERVICE(GameModeService, svc_game_mode);

DEFINE_HOOK(&dMeter2Draw_c::draw, dMeter2Draw_c__draw);

namespace HalloweenRace {
void setWeather() {
    std::string stageName = dComIfGp_getStartStageName();
    if (stageName.starts_with("F_SP") || stageName.starts_with("D_MN")) {
        dKyw_rain_set(250);
        g_env_light.mThunderEff.mMode = 1;

        if (g_env_light.mColpatWeather != 2) {
            g_env_light.mColpatWeather = 2;
            g_env_light.mColpatCurrGather = 2;
        }
    }
}

void fixJovaniPoe() {
    // this is extremely stupid but good enough for this mod's purpose
    static bool got_jovani_poe = false;
    static bool create_poe = false;

    if (!got_jovani_poe) {
        if (strcmp(dComIfGp_getStartStageName(), "R_SP160") == 0 && dComIfGp_getStartStageRoomNo() == 5) {
            dComIfGs_offSwitch(31, 5);

            if (!create_poe && daAlink_getAlinkActorClass() != nullptr) {
                const cXyz pos(4531.19f, -30.0f, 2631.961f);
                const csXyz angle(0, 0, 0);
                fopAcM_create(fpcNm_E_HP_e, 0xFF031F00, &pos, 5, &angle, nullptr, -1);
                create_poe = true;
            }

            if (daAlink_getAlinkActorClass() != nullptr && daAlink_getAlinkActorClass()->mProcID == daAlink_c::PROC_WOLF_DOWN_AT_LAND) {
                got_jovani_poe = true;
            }
        } else if (create_poe) {
            create_poe = false;
        }
    }
}

// this whole thing is just me being lazy
void hookPostMeter2Draw(ModContext*, void* args, void*, void*) {
    auto i_this = mods::arg<dMeter2Draw_c*>(args, 0);

    const f32 pos_x = i_this->mpLifeParent->getPanePtr()->getTranslateX();
    const f32 pos_y = i_this->mpLifeParent->getPanePtr()->getTranslateY();

    J2DTextBox poeCount;
    poeCount.setFont(mDoExt_getMesgFont());
    poeCount.setFontSize(16.0f, 16.0f);
    poeCount.setLineSpace(25.0f);

    char textbuf[256];
    SAFE_SPRINTF(textbuf, "Poes: %d/60", dComIfGs_getPohSpiritNum());
    poeCount.setString(textbuf);
    poeCount.draw(pos_x - 120.0f, pos_y - 30.0f);
}

ModResult onGameModeActivated(void*, ModError* error) {
    ModResult result = mods::hook::add_post<dMeter2Draw_c__draw>(hookPostMeter2Draw);
    if (result != MOD_OK)
        return result;

    return MOD_OK;
}

ModResult onGameModeDeactivated(void*, ModError* error) {
    ModResult result = mods::hook::uninstall<dMeter2Draw_c__draw>(svc_hook);
    if (result != MOD_OK)
        return result;

    return MOD_OK;
}

ModResult update(void*, ModError*) {
    setWeather();
    fixJovaniPoe();
    return MOD_OK;
}
}

extern "C" {
MOD_EXPORT ModResult mod_initialize(ModError*) {
    constexpr GameModeDesc gameModeDesc{
        .struct_size = sizeof(GameModeDesc),
        .game_mode_id = "halloween_race",
        .full_name = "Halloween Race",
        .save_name = "halloween_race",
        .user_data = nullptr,
        .on_activated = HalloweenRace::onGameModeActivated,
        .on_deactivated = HalloweenRace::onGameModeDeactivated,
        .on_tick = HalloweenRace::update,
    };

    ModResult result = svc_game_mode->register_game_mode(mod_ctx, &gameModeDesc);
    if (result != MOD_OK) {
        return result;
    }

    mods::log::info("halloween_race mod initialized");
    return MOD_OK;
}

MOD_EXPORT ModResult mod_update(ModError*) {
    return MOD_OK;
}

MOD_EXPORT ModResult mod_shutdown(ModError*) {
    return MOD_OK;
}
}
