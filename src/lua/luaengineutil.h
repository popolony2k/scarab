/*
 * luaengineutil.h
 *
 *  Created on: Jul 6, 2026
 *      Author: popolony2k
 */

#ifndef __LUAENGINEUTIL_H__
#define __LUAENGINEUTIL_H__

#include <cstddef>
#include <cstdint>
#include <map>
#include <mutex>

extern "C"
{
  #include "lua.h"
}

#include "tilemap/itilemap.h"
#include "sound/soundmanager.h"
#include "engine/spritepool.h"
#include "scripting/scriptprocessor.h"
#include "concurrent/timer.h"


namespace Scarab  {
    namespace Engine  {
        namespace Lua  {

            /**
             * @brief A named integer constant, used to register enum values as Lua globals.
             */
            struct stNamedConstant  {
                const char  *szName;
                int         nValue;
            };

            /**
             * @brief Shared type for LuaEngine's m_Timers member and LuaTimerApi's
             * casts of the timerMapPtr global - both need the exact same type,
             * not just structurally identical ones.
             */
            typedef std :: map<int, SunLight :: Concurrent :: Timer*> TimerMap;

            /**
             * @brief Shared helpers used by every Lua primitive API module -
             * and, for s_LuaMutex specifically, by LuaEngine itself too
             * (the orchestrator, not one of the primitive API modules -
             * see s_LuaMutex's own comment for why it lives here rather
             * than on LuaEngine).
             */
            class LuaEngineUtil  {

                public:

                /*
                 * SunLight::Concurrent::Timer::Start() runs it's callback on
                 * it's own background thread (see LuaTimerApi::SetTimer), and
                 * that callback calls lua_pcall directly on the shared
                 * lua_State - lua_State is not thread-safe, so every entry
                 * point that can touch it from the main thread
                 * (LuaEngine::CallOnUpdate/TryDispatchMoveSpritesToScreen/
                 * TryDispatchLoadStage/GetActiveEnemyCount/RunFile) must
                 * serialize against the timer thread through this same
                 * mutex, or the two threads racing on the interpreter's
                 * internal state corrupts it and crashes (seen in practice as
                 * a SIGSEGV inside Lua's own error-formatting code).
                 * Lua-callable static functions (LuaScriptingApi::Wait,
                 * LuaSpriteApi::*, etc.) do NOT need to lock it themselves -
                 * they only ever run reentrantly on the main thread, already
                 * inside a lua_pcall an entry point above is holding the lock
                 * for. The timer callback itself must use try_lock rather
                 * than a blocking lock: LuaTimerApi::ResetTimer runs
                 * reentrantly on the main thread and calls Timer::Stop(),
                 * which joins the timer thread - if the timer thread were
                 * blocked waiting on this same mutex at that moment, the join
                 * would deadlock against the thread that holds it. Lives here
                 * (not on LuaEngine) since LuaTimerApi needs it too, and this
                 * is the one class every Lua primitive API module already
                 * depends on.
                 */
                static std :: mutex                       s_LuaMutex;

                static SunLight :: TileMap :: ITileMap* GetTileMap( lua_State *pLuaState );
                static SunLight :: Sound :: SoundManager* GetSoundManager( lua_State *pLuaState );
                static Engine :: SpritePool* GetSpritePool( lua_State *pLuaState );
                static SunLight :: Scripting :: ScriptProcessor* GetScriptProcessor( lua_State *pLuaState );
                static void AddOneParmCommandScript( lua_State *pLuaState, SunLight :: Scripting :: Commands cmd );
                static void RegisterConstants( lua_State *pLuaState, const stNamedConstant *pTable, size_t nCount );
            };
        }
    }
}

#endif  /* __LUAENGINEUTIL_H__ */
