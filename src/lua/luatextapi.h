/*
 * luatextapi.h
 *
 *  Created on: Aug 14, 2026
 *      Author: popolony2k
 */

#ifndef __LUATEXTAPI_H__
#define __LUATEXTAPI_H__

extern "C"
{
  #include "lua.h"
}


namespace Scarab  {
    namespace Engine  {
        namespace Lua  {
            /**
             * @brief Text/font Lua primitives - a thin wrapper over
             * ITileMap::SetFont/DrawText/MeasureText/GetWindowWidth/
             * GetWindowHeight. All game-side HUD logic (what to draw,
             * when, positioning, score/lives formatting) lives in Lua on
             * top of these; this class knows nothing about what the text
             * being drawn actually means.
             */
            class LuaTextApi  {

                static int SetFont( lua_State *pLuaState );
                static int DrawText( lua_State *pLuaState );
                static int MeasureText( lua_State *pLuaState );
                static int GetWindowWidth( lua_State *pLuaState );
                static int GetWindowHeight( lua_State *pLuaState );

                public:

                static void Register( lua_State *pLuaState );
            };
        }
    }
}

#endif  /* __LUATEXTAPI_H__ */
