/*
    C-Dogs SDL PicOS Port — Entry Point
    Phase 1: Boot to main menu on PicOS 320x320 display
*/
#include "app_abi.h"
#include "os.h"
#include "picos_sdl_impl.h"
#include <string.h>
#include <setjmp.h>

/* C-Dogs headers */
#include "cdogs/config.h"
#include "cdogs/pic_manager.h"
#include "cdogs/grafx.h"
#include "cdogs/font.h"
#include "cdogs/font_utils.h"
#include "cdogs/sounds.h"
#include "cdogs/events.h"
#include "cdogs/draw/char_sprites.h"
#include "cdogs/particle.h"
#include "cdogs/ammo.h"
#include "cdogs/bullet_class.h"
#include "cdogs/weapon_class.h"
#include "cdogs/character_class.h"
#include "cdogs/pickup_class.h"
#include "cdogs/map_object.h"
#include "cdogs/collision/collision.h"
#include "cdogs/campaigns.h"
#include "cdogs/player.h"
#include "cdogs/player_template.h"
#include "loading_screens.h"
#include "autosave.h"
#include "mainmenu.h"
#include "game_loop.h"

/* Global PicOS state (referenced by stubs.c) */
const PicoCalcAPI *g_picos_api;
char g_app_dir[128];
jmp_buf g_exit_jmp;

void picos_main(const PicoCalcAPI *api,
                const char *app_dir,
                const char *app_id,
                const char *app_name)
{
    (void)app_id;
    (void)app_name;

    g_picos_api = api;
    strncpy(g_app_dir, app_dir, sizeof(g_app_dir) - 1);
    g_app_dir[sizeof(g_app_dir) - 1] = '\0';

    api->sys->log("CDOGS: Starting Phase 1...");

    /* Init PicOS SDL layer */
    picos_sdl_init(api);

    /* If C-Dogs calls exit(), longjmp back here */
    int exit_code = setjmp(g_exit_jmp);
    if (exit_code != 0) {
        api->sys->log("CDOGS: exit() called, returning to launcher");
        return;
    }

    /* Configure for 320x240 (minimum C-Dogs resolution) */
    gConfig = ConfigDefault();
    ConfigSetInt(&gConfig, "Graphics.WindowWidth", 320);
    ConfigSetInt(&gConfig, "Graphics.WindowHeight", 240);
    ConfigSetInt(&gConfig, "Graphics.ScaleFactor", 1);
    /* Fullscreen is CONFIG_TYPE_BOOL, not INT — default is already false */
    ConfigGet(&gConfig, "Graphics.Fullscreen")->u.Bool.Value = false;
    ConfigSetInt(&gConfig, "Sound.SoundVolume", 0);
    ConfigSetInt(&gConfig, "Sound.MusicVolume", 0);

    api->sys->log("CDOGS: Config set to 320x240");

    /* Core init sequence (mirrors cdogs.c:main) */
    PicManagerInit(&gPicManager);
    api->sys->log("CDOGS: PicManager initialized");

    GraphicsInit(&gGraphicsDevice, &gConfig);
    api->sys->log("CDOGS: GraphicsInit done");

    GraphicsInitialize(&gGraphicsDevice);
    api->sys->log("CDOGS: GraphicsInitialize done");

    if (!gGraphicsDevice.IsInitialized) {
        api->sys->log("CDOGS: ERROR - Graphics failed to initialize!");
        return;
    }

    FontLoadFromJSON(&gFont, "graphics/font.png", "graphics/font.json");
    api->sys->log("CDOGS: Font loaded");

    api->sys->log("CDOGS: About to call LoadingScreenInit");
    LoadingScreenInit(&gLoadingScreen, &gGraphicsDevice);
    api->sys->log("CDOGS: LoadingScreenInit done");

    api->sys->log("CDOGS: About to call LoadingScreenDraw");
    LoadingScreenDraw(&gLoadingScreen, "Loading graphics...", 0.0f);
    api->sys->log("CDOGS: LoadingScreenDraw done");

    api->sys->log("CDOGS: About to call PicManagerLoad");
    {
        char buf[CDOGS_PATH_MAX];
        GetDataFilePath(buf, "graphics");
        fprintf(stderr, "CDOGS: PicManagerLoad path='%s'\n", buf);
        api->sys->log("CDOGS: About to call PicManagerLoadDir");
        PicManagerLoadDir(&gPicManager, buf, NULL, gPicManager.pics, gPicManager.sprites, false);
        api->sys->log("CDOGS: PicManagerLoadDir returned for graphics");
        GetDataFilePath(buf, "graphics_hd");
        PicManagerLoadDir(&gPicManager, buf, NULL, gPicManager.pics, gPicManager.sprites, true);
        api->sys->log("CDOGS: PicManagerLoadDir returned for graphics_hd");
    }
    api->sys->log("CDOGS: PicManager loaded");

    LoadingScreenDraw(&gLoadingScreen, "Loading autosaves...", 0.1f);
    AutosaveInit(&gAutosave);
    api->sys->log("CDOGS: Autosave initialized");

    LoadingScreenDraw(&gLoadingScreen, "Initializing sound...", 0.25f);
    SoundInitialize(&gSoundDevice, "sounds");
    api->sys->log("CDOGS: Sound initialized");

    EventInit(&gEventHandlers);
    api->sys->log("CDOGS: Events initialized");

    api->sys->log("CDOGS: About to CharSpriteClassesInit");
    CharSpriteClassesInit(&gCharSpriteClasses);
    api->sys->log("CDOGS: CharSpriteClasses done");

    api->sys->log("CDOGS: About to ParticleClassesInit");
    ParticleClassesInit(&gParticleClasses, "data/particles.json");
    api->sys->log("CDOGS: Particles done");

    api->sys->log("CDOGS: About to AmmoInitialize");
    AmmoInitialize(&gAmmo, "data/ammo.json");
    api->sys->log("CDOGS: Ammo done");

    api->sys->log("CDOGS: About to BulletAndWeaponInitialize");
    BulletAndWeaponInitialize(
        &gBulletClasses, &gWeaponClasses,
        "data/bullets.json", "data/guns.json");
    api->sys->log("CDOGS: Weapons done");

    api->sys->log("CDOGS: About to CharacterClassesInitialize");
    CharacterClassesInitialize(&gCharacterClasses, "data/character_classes.json");
    api->sys->log("CDOGS: Characters done");

    api->sys->log("CDOGS: About to PlayerTemplatesLoad");
    PlayerTemplatesLoad(&gPlayerTemplates, &gCharacterClasses);
    api->sys->log("CDOGS: Templates done");

    api->sys->log("CDOGS: About to PickupClassesInit");
    PickupClassesInit(
        &gPickupClasses, "data/pickups.json", &gAmmo, &gWeaponClasses);
    api->sys->log("CDOGS: Pickups done");

    api->sys->log("CDOGS: About to MapObjectsInit");
    MapObjectsInit(
        &gMapObjects, "data/map_objects.json", &gAmmo, &gWeaponClasses);
    api->sys->log("CDOGS: MapObjects done");

    CollisionSystemInit(&gCollisionSystem);
    CampaignInit(&gCampaign);
    PlayerDataInit(&gPlayerDatas);

    api->sys->log("CDOGS: All game data loaded");

    /* Main menu loop */
    LoadingScreenDraw(&gLoadingScreen, "Starting...", 1.0f);
    LoopRunner l = LoopRunnerNew();
    LoopRunnerPush(&l, MainMenu(&gGraphicsDevice, &l));
    api->sys->log("CDOGS: Entering main menu loop");
    LoopRunnerRun(&l);

    /* Cleanup */
    api->sys->log("CDOGS: Main loop exited, cleaning up...");
    LoopRunnerTerminate(&l);

    api->sys->log("CDOGS: Exiting");
}
