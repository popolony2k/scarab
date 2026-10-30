/*
 * Copyright (c) since 2021 by PopolonY2k and Leidson Campos A. Ferreira
 * 
 * This software is provided 'as-is', without any express or implied
 * warranty. In no event will the authors be held liable for any damages
 * arising from the use of this software.
 *
 * Permission is granted to anyone to use this software for any purpose,
 * including commercial applications, and to alter it and redistribute it
 * freely, subject to the following restrictions:
 *
 * 1. The origin of this software must not be misrepresented; you must not
 * claim that you wrote the original software. If you use this software
 * in a product, an acknowledgment in the product documentation would be
 * appreciated but is not required.
 * 2. Altered source versions must be plainly marked as such, and must not be
 * misrepresented as being the original software.
 * 3. This notice may not be removed or altered from any source distribution.
 */

#include "lua/luaengineutil.h"
#include <climits>
#include <cstdio>
#include <cstdlib>

/** The most significant digits a float needs to round-trip through decimal text. */
#define __FLOAT_ROUND_TRIP_DIGITS  9


namespace Scarab  {
    namespace Engine  {
        namespace Lua  {

            std :: mutex LuaEngineUtil :: s_LuaMutex;

            /**
             * @brief Fetch the renderer provider stashed as a Lua light userdata global - the
             * one object every renderer-needing primitive reaches the renderer through, since
             * the renderer does not exist yet when the Lua engine is built (see
             * IRendererProvider).
             *
             * @param pLuaState Lua state to be used by engine call.
             */
            Engine :: IRendererProvider* LuaEngineUtil :: GetRendererProvider( lua_State *pLuaState )  {

                lua_getglobal( pLuaState, "rendererProviderPtr" );

                Engine :: IRendererProvider  *pProvider = static_cast<Engine :: IRendererProvider *>( lua_touserdata( pLuaState, -1 ) );

                lua_pop( pLuaState, 1 );

                return pProvider;
            }

            /**
             * @brief Name of the Lua-callable primitive currently running (as the script called
             * it, eg. "app_set_name") - recorded by the provider when this call is what creates
             * the default renderer, so a later renderer_create can name the culprit.
             */
            static std :: string GetCallerName( lua_State *pLuaState )  {

                lua_Debug  ar;

                if( lua_getstack( pLuaState, 0, &ar ) && lua_getinfo( pLuaState, "n", &ar ) && ar.name )
                    return ar.name;

                return "(unknown primitive)";
            }

            /**
             * @brief Make sure a renderer exists, creating the default one (and opening its
             * window) if the script hasn't - for primitives that need a live window/GL context
             * but don't go through GetTileMap()/GetDrawSurface() (eg. sprite_configure_texture,
             * which loads a GPU texture).
             *
             * @param pLuaState Lua state to be used by engine call.
             */
            void LuaEngineUtil :: EnsureRenderer( lua_State *pLuaState )  {

                GetRendererProvider( pLuaState ) -> GetTileMap( GetCallerName( pLuaState ).c_str() );
            }

            /**
             * @brief The engine's ITileMap - creating the default renderer first if the script
             * has not created one yet (see IRendererProvider::GetTileMap).
             *
             * @param pLuaState Lua state to be used by engine call.
             */
            SunLight :: TileMap :: ITileMap* LuaEngineUtil :: GetTileMap( lua_State *pLuaState )  {

                return GetRendererProvider( pLuaState ) -> GetTileMap( GetCallerName( pLuaState ).c_str() );
            }

            /**
             * @brief The engine's IDrawSurface - screen-space text/rectangle drawing and
             * window-state primitives (see sunlight's own IDrawSurface header comment), a
             * separate interface from GetTileMap()'s since sunlight v0.12.0 split those off
             * ITileMap. Both address the same underlying TileMapRenderer object today (it
             * implements both), but callers should reach for whichever interface actually
             * matches what they're doing, not assume that always holds. Creates the default
             * renderer first if the script has not created one yet.
             *
             * @param pLuaState Lua state to be used by engine call.
             */
            SunLight :: DrawSurface :: IDrawSurface* LuaEngineUtil :: GetDrawSurface( lua_State *pLuaState )  {

                return GetRendererProvider( pLuaState ) -> GetDrawSurface( GetCallerName( pLuaState ).c_str() );
            }

            /**
             * @brief Fetch the engine's SoundManager instance stashed as a Lua light userdata global.
             *
             * @param pLuaState Lua state to be used by engine call.
             */
            SunLight :: Sound :: SoundManager* LuaEngineUtil :: GetSoundManager( lua_State *pLuaState )  {

                lua_getglobal( pLuaState, "soundManagerPtr" );

                SunLight :: Sound :: SoundManager  *pObject = static_cast<SunLight :: Sound :: SoundManager *>( lua_touserdata( pLuaState, -1 ) );

                lua_pop( pLuaState, 1 );

                return pObject;
            }

            /**
             * @brief Fetch the engine's SpritePool instance stashed as a Lua light userdata global.
             *
             * @param pLuaState Lua state to be used by engine call.
             */
            Engine :: SpritePool* LuaEngineUtil :: GetSpritePool( lua_State *pLuaState )  {

                lua_getglobal( pLuaState, "spritePoolPtr" );

                Engine :: SpritePool  *pObject = static_cast<Engine :: SpritePool *>( lua_touserdata( pLuaState, -1 ) );

                lua_pop( pLuaState, 1 );

                return pObject;
            }

            /**
             * @brief Fetch the engine's ScriptProcessor instance stashed as a Lua light userdata global.
             *
             * @param pLuaState Lua state to be used by engine call.
             */
            SunLight :: Scripting :: ScriptProcessor* LuaEngineUtil :: GetScriptProcessor( lua_State *pLuaState )  {

                lua_getglobal( pLuaState, "scriptProcessorPtr" );

                SunLight :: Scripting :: ScriptProcessor  *pObject = static_cast<SunLight :: Scripting :: ScriptProcessor *>( lua_touserdata( pLuaState, -1 ) );

                lua_pop( pLuaState, 1 );

                return pObject;
            }

            /**
             * @brief Add an one parameter command to the ScriptProcessor's queue;
             * shared by every Lua primitive that queues a command rather than
             * acting immediately (sp_wait, sp_move_sprites_to_screen,
             * sp_play_song, ...).
             *
             * The parameter is the first argument, an integer the command's 16 bit slot can hold
             * (0..65535) - anything else is an error and nothing is queued.
             *
             * @param pLuaState Lua state to be used by engine call.
             * @param cmd The command to add to queue;
             * @param szName The parameter's name, for the error message;
             * @param strError Receives the reason on failure;
             * @return true if the command was queued;
             */
            bool LuaEngineUtil :: AddOneParmCommandScript( lua_State *pLuaState, SunLight :: Scripting :: Commands cmd,
                                                           const char *szName, std :: string &strError )  {

                lua_Integer  nParm = 0;

                if( !ReadIntegerArgInRange( pLuaState, 1, szName, 0, UINT16_MAX, nParm, strError ) )
                    return false;

                GetScriptProcessor( pLuaState ) -> AddOneParmCmd( cmd, ( uint16_t ) nParm );

                return true;
            }

            /**
             * @brief Push `nil, message` and return the count - the failure convention of the
             * Lua primitives that report errors (doc/MASTER_PLAN.md items B1, D3): a script reads
             * it with `local ok, err = f( ... )`, and it never raises a Lua error.
             */
            int LuaEngineUtil :: PushError( lua_State *pLuaState, const std :: string &strMessage )  {

                lua_pushnil( pLuaState );
                lua_pushstring( pLuaState, strMessage.c_str() );

                return 2;
            }

            /** @brief Push `true` - the success value that goes with @link PushError. */
            int LuaEngineUtil :: PushTrue( lua_State *pLuaState )  {

                lua_pushboolean( pLuaState, 1 );

                return 1;
            }

            /**
             * @brief Format a number the way a user would write it (3.8125, 16) - for error
             * messages that quote a value or a limit.
             */
            std :: string LuaEngineUtil :: FormatNumber( double fValue )  {

                char  szBuffer[64];

                snprintf( szBuffer, sizeof( szBuffer ), "%g", fValue );

                return szBuffer;
            }

            /**
             * @brief The shortest decimal that converts back to exactly this float, as a double - so an
             * authored opacity of 0.69 (stored as the float 0.6899999761...) reaches Lua as 0.69, as Tiled
             * shows it, and writing that back gives the identical float again. Used by every getter that
             * returns a value sunlight keeps as a float (a layer's or a sprite's opacity).
             */
            double LuaEngineUtil :: ShortestDecimal( float fValue )  {

                char  szBuffer[32];

                for( int nDigits = 1; nDigits < __FLOAT_ROUND_TRIP_DIGITS; nDigits++ )  {
                    snprintf( szBuffer, sizeof( szBuffer ), "%.*g", nDigits, ( double ) fValue );

                    if( ( float ) strtod( szBuffer, nullptr ) == fValue )
                        return strtod( szBuffer, nullptr );
                }

                return ( double ) fValue;
            }

            /*
             * The argument readers below are the strict argument checks of every Lua primitive
             * (doc/MASTER_PLAN.md item D3): each reads one argument, and on a wrong type or value
             * returns false with the reason in strError, which the caller reports as
             * `nil, message` through PushError - never as a Lua error. The messages all start
             * "argument '<name>'", so a script sees which argument was wrong.
             */

            /**
             * @brief Read a required integer argument: a number with an exact integer value (3 and
             * 3.0 are accepted; 3.5, a string or a missing argument are not).
             */
            bool LuaEngineUtil :: ReadIntegerArg( lua_State *pLuaState, int nIndex, const char *szName, lua_Integer &nOut, std :: string &strError )  {

                int  bIsInteger = 0;

                if( lua_type( pLuaState, nIndex ) == LUA_TNUMBER )
                    nOut = lua_tointegerx( pLuaState, nIndex, &bIsInteger );

                if( !bIsInteger )
                    strError = std :: string( "argument '" ) + szName + "' must be an integer";

                return bIsInteger != 0;
            }

            /** @brief Read a required integer argument that must lie in [nMin, nMax]. */
            bool LuaEngineUtil :: ReadIntegerArgInRange( lua_State *pLuaState, int nIndex, const char *szName, lua_Integer nMin, lua_Integer nMax,
                                                         lua_Integer &nOut, std :: string &strError )  {

                if( !ReadIntegerArg( pLuaState, nIndex, szName, nOut, strError ) )
                    return false;

                if( nOut < nMin || nOut > nMax )  {
                    strError = std :: string( "argument '" ) + szName + "' must be between " + std :: to_string( nMin ) +
                        " and " + std :: to_string( nMax ) + ", got " + std :: to_string( nOut );
                    return false;
                }

                return true;
            }

            /** @brief Read a required integer argument that must fit a C++ int. */
            bool LuaEngineUtil :: ReadIntArg( lua_State *pLuaState, int nIndex, const char *szName, int &nOut, std :: string &strError )  {

                lua_Integer  nValue = 0;

                if( !ReadIntegerArgInRange( pLuaState, nIndex, szName, INT_MIN, INT_MAX, nValue, strError ) )
                    return false;

                nOut = ( int ) nValue;

                return true;
            }

            /** @brief Read an optional integer argument: none or nil gives nDefault. */
            bool LuaEngineUtil :: ReadOptionalIntegerArg( lua_State *pLuaState, int nIndex, const char *szName, lua_Integer nDefault,
                                                          lua_Integer &nOut, std :: string &strError )  {

                if( lua_isnoneornil( pLuaState, nIndex ) )  {
                    nOut = nDefault;
                    return true;
                }

                return ReadIntegerArg( pLuaState, nIndex, szName, nOut, strError );
            }

            /** @brief Read a required number argument (an integer or a real). */
            bool LuaEngineUtil :: ReadNumberArg( lua_State *pLuaState, int nIndex, const char *szName, double &fOut, std :: string &strError )  {

                if( lua_type( pLuaState, nIndex ) != LUA_TNUMBER )  {
                    strError = std :: string( "argument '" ) + szName + "' must be a number";
                    return false;
                }

                fOut = lua_tonumber( pLuaState, nIndex );

                return true;
            }

            /** @brief Read a required number argument that must lie in [fMin, fMax]. */
            bool LuaEngineUtil :: ReadNumberArgInRange( lua_State *pLuaState, int nIndex, const char *szName, double fMin, double fMax,
                                                        double &fOut, std :: string &strError )  {

                if( !ReadNumberArg( pLuaState, nIndex, szName, fOut, strError ) )
                    return false;

                if( !( fOut >= fMin && fOut <= fMax ) )  {
                    strError = std :: string( "argument '" ) + szName + "' must be between " + FormatNumber( fMin ) +
                        " and " + FormatNumber( fMax ) + ", got " + FormatNumber( fOut );
                    return false;
                }

                return true;
            }

            /** @brief Read a required boolean argument (true or false - not nil, not 0/1). */
            bool LuaEngineUtil :: ReadBooleanArg( lua_State *pLuaState, int nIndex, const char *szName, bool &bOut, std :: string &strError )  {

                if( lua_type( pLuaState, nIndex ) != LUA_TBOOLEAN )  {
                    strError = std :: string( "argument '" ) + szName + "' must be a boolean";
                    return false;
                }

                bOut = lua_toboolean( pLuaState, nIndex );

                return true;
            }

            /** @brief Read an optional boolean argument: none or nil gives bDefault. */
            bool LuaEngineUtil :: ReadOptionalBooleanArg( lua_State *pLuaState, int nIndex, const char *szName, bool bDefault,
                                                          bool &bOut, std :: string &strError )  {

                if( lua_isnoneornil( pLuaState, nIndex ) )  {
                    bOut = bDefault;
                    return true;
                }

                return ReadBooleanArg( pLuaState, nIndex, szName, bOut, strError );
            }

            /** @brief Read a required string argument (a real string - a number is not converted). */
            bool LuaEngineUtil :: ReadStringArg( lua_State *pLuaState, int nIndex, const char *szName, std :: string &strOut, std :: string &strError )  {

                if( lua_type( pLuaState, nIndex ) != LUA_TSTRING )  {
                    strError = std :: string( "argument '" ) + szName + "' must be a string";
                    return false;
                }

                size_t       nLength = 0;
                const char  *szValue = lua_tolstring( pLuaState, nIndex, &nLength );

                strOut.assign( szValue, nLength );

                return true;
            }

            /** @brief Check that a required argument is a function. */
            bool LuaEngineUtil :: CheckFunctionArg( lua_State *pLuaState, int nIndex, const char *szName, std :: string &strError )  {

                if( !lua_isfunction( pLuaState, nIndex ) )  {
                    strError = std :: string( "argument '" ) + szName + "' must be a function";
                    return false;
                }

                return true;
            }

            /**
             * @brief Read a required argument that must be one of a constant group's values (eg. a
             * KEY_* or a MAP_ALIGNMENT_* constant), checked against the same table that registers
             * the group's Lua globals.
             *
             * @param pTable, nCount The group's constants;
             * @param szGroup The group's name pattern for the message (eg. "KEY_*");
             */
            bool LuaEngineUtil :: ReadConstantArg( lua_State *pLuaState, int nIndex, const char *szName, const stNamedConstant *pTable,
                                                   size_t nCount, const char *szGroup, int &nOut, std :: string &strError )  {

                lua_Integer  nValue = 0;

                if( !ReadIntegerArg( pLuaState, nIndex, szName, nValue, strError ) )  {
                    strError = std :: string( "argument '" ) + szName + "' must be a " + szGroup + " constant";
                    return false;
                }

                for( size_t nItem = 0; nItem < nCount; nItem++ )  {
                    if( pTable[nItem].nValue == nValue )  {
                        nOut = ( int ) nValue;
                        return true;
                    }
                }

                strError = std :: string( "argument '" ) + szName + "': unknown " + szGroup + " value " + std :: to_string( nValue );

                return false;
            }

            /**
             * @brief Register a table of named integer constants as Lua globals.
             *
             * @param pLuaState Lua state to be used by engine call.
             * @param pTable The constants table to register;
             * @param nCount Number of entries in the table;
             */
            void LuaEngineUtil :: RegisterConstants( lua_State *pLuaState, const stNamedConstant *pTable, size_t nCount )  {

                for( size_t nIndex = 0; nIndex < nCount; nIndex++ )  {
                    lua_pushinteger( pLuaState, pTable[nIndex].nValue );
                    lua_setglobal( pLuaState, pTable[nIndex].szName );
                }
            }

            /**
             * @brief Register a table of named real-valued constants as Lua globals (see
             * stNamedRealConstant).
             *
             * @param pLuaState Lua state to be used by engine call.
             * @param pTable The constants table to register;
             * @param nCount Number of entries in the table;
             */
            void LuaEngineUtil :: RegisterRealConstants( lua_State *pLuaState, const stNamedRealConstant *pTable, size_t nCount )  {

                for( size_t nIndex = 0; nIndex < nCount; nIndex++ )  {
                    lua_pushnumber( pLuaState, pTable[nIndex].fValue );
                    lua_setglobal( pLuaState, pTable[nIndex].szName );
                }
            }
        }
    }
}
