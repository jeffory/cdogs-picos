/*
    C-Dogs SDL - PicOS sys_config.h
    Manually generated (replaces CMake-generated version)
*/
#pragma once

#define CDOGS_SDL_VERSION "v0.0.1-picos"

/* where to look for the cdogs data files */
#define CDOGS_DATA_DIR "/apps/cdogs/data/"

#define CDOGS_CFG_DIR "/data/com.picos.cdogs"

#define CDOGS_CAMPAIGN_DIR "missions"
#define CDOGS_DOGFIGHT_DIR "dogfights"

#define CDOGS_FILENAME_MAX 128
#define CDOGS_PATH_MAX 256

#define FPS_FRAMELIMIT 30
#define PICKUP_LIMIT (FPS_FRAMELIMIT * 5)
