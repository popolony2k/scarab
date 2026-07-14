/*
 * main.h
 *
 *  Created on: Sep 1, 2021
 *      Author: popolony2k
 */

#ifndef __MAIN_H__
#define __MAIN_H__

#define DISPLAY_W                 1260
#define DISPLAY_H                 920
#define FRAMES_PER_SECOND         60
#define H_SCROLL_STEP_SIZE        1
#define W_SCROLL_STEP_SIZE        1
#define VIEWPORT_POS_X            10
#define VIEWPORT_POS_Y            10
#define VIEWPORT_WIDTH            1250
#define VIEWPORT_HEIGHT           910
#define DEFAULT_ZOOM_SCALE_POS    60

/*
 * Scarab is a game-agnostic engine - it doesn't hardcode a specific
 * game's name. "Scarab" is only the initial window title, shown before
 * any Lua exists to override it; the actual game (main.lua) is expected
 * to call app_set_name(...) with its own name (e.g. "Caravellius") once
 * it starts running.
 */
#define APP_NAME                  "Scarab"

#endif /* __MAIN_H__ */
