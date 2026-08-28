/*
 * luaengine.cpp
 *
 *  Created on: Dec 13, 2023
 *      Author: popolony2k
 */

#include "lua/luaengine.h"
#include "lua/luajsonapi.h"
#include "lua/luafilesystemapi.h"
#include "lua/luaappapi.h"
#include "lua/luatextapi.h"
#include "lua/luacameraapi.h"
#include "lua/luainputapi.h"
#include "lua/luatilemapapi.h"
#include "lua/luasoundapi.h"
#include "lua/luaspriteapi.h"
#include "lua/luacollisionapi.h"
#include "lua/luacollisionlistener.h"
#include "lua/luascriptingapi.h"
#include "lua/luatimerapi.h"
#include "tilemap/itilemap.h"
#include "sound/soundmanager.h"
#include "engine/spritepool.h"
#include "engines/enginefactory.h"
#include "filesystem/filesystemfactory.h"
#include <vector>


namespace Scarab  {
  namespace Lua {

    /**
     * @brief Register all C++ calls to lua engine;
     *
     */
    void LuaEngine :: RegisterCalls( void )  {

        // Add all globals to lua engine
        lua_pushlightuserdata( m_pLuaState, m_pScriptProcessor );
        lua_setglobal( m_pLuaState, "scriptProcessorPtr" );

        lua_pushlightuserdata( m_pLuaState, &m_Timers );
        lua_setglobal( m_pLuaState, "timerMapPtr" );

        // Register the primitive API modules that only need
        // scriptProcessorPtr/timerMapPtr (both set just above) - everything
        // else (camera/input/tilemap/sound/sprite/collision/app) needs
        // ITileMap/SoundManager/SpritePool, not available until Init().
        Engine :: Lua :: LuaScriptingApi :: Register( m_pLuaState );
        Engine :: Lua :: LuaTimerApi :: Register( m_pLuaState );
        Engine :: Lua :: LuaJsonApi :: Register( m_pLuaState );

        // Overrides Lua's own built-in dofile() - must run after
        // luaL_openlibs() (guaranteed, since RegisterCalls() itself is
        // only ever called right after that) so this replaces the
        // standard library's version rather than being replaced by it.
        Engine :: Lua :: LuaFileSystemApi :: Register( m_pLuaState );
    }

    /**
     * @brief Call the optional Lua on_update(dt) global once per frame, if defined.
     * Absent global is a silent no-op so stages that don't define it keep working.
     *
     * @param nDeltaMilli Milliseconds elapsed since the last frame;
     */
    void LuaEngine :: CallOnUpdate( uint32_t nDeltaMilli )  {

        std :: lock_guard<std :: mutex>  lock( Engine :: Lua :: LuaEngineUtil :: s_LuaMutex );

        lua_getglobal( m_pLuaState, "on_update" );

        if( !lua_isfunction( m_pLuaState, -1 ) )  {
            lua_pop( m_pLuaState, 1 );

            return;
        }

        lua_pushinteger( m_pLuaState, nDeltaMilli );

        if( lua_pcall( m_pLuaState, 1, 0, 0 ) != 0 )  {
            fprintf( stderr, "Error calling on_update: %s\n", lua_tostring( m_pLuaState, -1 ) );
            lua_pop( m_pLuaState, 1 );
        }
    }

    /**
     * @brief Dispatch a MOVE_SPRITES_TO_SCREEN_CMD state id to Lua, via the
     * optional global on_move_sprites_to_screen(stateId). The state id
     * itself is entirely Lua-owned (see src/wavestates.lua) -
     * C++ just ferries the ScriptProcessor's queued integer back to Lua
     * opaquely, so stage scripts' sp_move_sprites_to_screen(...) calls (and
     * their sp_wait/sp_wait_queue_empty pacing) work the same regardless of
     * which enemy type a given id belongs to.
     *
     * @param nStateId The wave-spawn state id received from the queue;
     * @return true if Lua claimed and handled this state id; false if Lua
     * doesn't handle it (no global defined, or the handler explicitly
     * declines, or no wave handler registered for this id).
     */
    bool LuaEngine :: TryDispatchMoveSpritesToScreen( uint16_t nStateId )  {

        std :: lock_guard<std :: mutex>  lock( Engine :: Lua :: LuaEngineUtil :: s_LuaMutex );

        lua_getglobal( m_pLuaState, "on_move_sprites_to_screen" );

        if( !lua_isfunction( m_pLuaState, -1 ) )  {
            lua_pop( m_pLuaState, 1 );

            return false;
        }

        lua_pushinteger( m_pLuaState, nStateId );

        if( lua_pcall( m_pLuaState, 1, 1, 0 ) != 0 )  {
            fprintf( stderr, "Error calling on_move_sprites_to_screen: %s\n", lua_tostring( m_pLuaState, -1 ) );
            lua_pop( m_pLuaState, 1 );

            return false;
        }

        bool  bHandled = lua_toboolean( m_pLuaState, -1 );

        lua_pop( m_pLuaState, 1 );

        return bHandled;
    }

    /**
     * @brief Give Lua first refusal on a LOAD_STAGE_CMD state id, via the
     * optional Lua global on_load_stage(stageId) (bootstrap.lua) - the same
     * "Lua gets first refusal" shape as TryDispatchMoveSpritesToScreen. Since
     * stage bootstrap is a single mechanism (not per-type like the enemy
     * ports), there is no legacy C++ fallback here - Lua is expected to
     * always claim a valid stage id.
     *
     * @param nStageId The stage id (src/stageids.lua) received from the queue;
     * @return true if Lua loaded the stage's map and script successfully.
     */
    bool LuaEngine :: TryDispatchLoadStage( uint16_t nStageId )  {

        std :: lock_guard<std :: mutex>  lock( Engine :: Lua :: LuaEngineUtil :: s_LuaMutex );

        lua_getglobal( m_pLuaState, "on_load_stage" );

        if( !lua_isfunction( m_pLuaState, -1 ) )  {
            lua_pop( m_pLuaState, 1 );

            return false;
        }

        lua_pushinteger( m_pLuaState, nStageId );

        if( lua_pcall( m_pLuaState, 1, 1, 0 ) != 0 )  {
            fprintf( stderr, "Error calling on_load_stage: %s\n", lua_tostring( m_pLuaState, -1 ) );
            lua_pop( m_pLuaState, 1 );

            return false;
        }

        bool  bHandled = lua_toboolean( m_pLuaState, -1 );

        lua_pop( m_pLuaState, 1 );

        return bHandled;
    }

    /**
     * @brief Ask Lua how many enemies it currently has active, via the
     * optional global get_active_enemy_count(). EngineHost::CheckSpritesQueueEmpty's
     * "is the screen clear, unblock sp_wait_queue_empty()" check is entirely
     * driven by this - every enemy type is Lua/SpritePool-owned, so this
     * count is the only source of truth for whether the screen is clear.
     *
     * @return The Lua-reported active enemy count, or 0 if no such global is defined.
     */
    int LuaEngine :: GetActiveEnemyCount( void )  {

        std :: lock_guard<std :: mutex>  lock( Engine :: Lua :: LuaEngineUtil :: s_LuaMutex );

        lua_getglobal( m_pLuaState, "get_active_enemy_count" );

        if( !lua_isfunction( m_pLuaState, -1 ) )  {
            lua_pop( m_pLuaState, 1 );

            return 0;
        }

        if( lua_pcall( m_pLuaState, 0, 1, 0 ) != 0 )  {
            fprintf( stderr, "Error calling get_active_enemy_count: %s\n", lua_tostring( m_pLuaState, -1 ) );
            lua_pop( m_pLuaState, 1 );

            return 0;
        }

        int  nCount = ( int ) lua_tointeger( m_pLuaState, -1 );

        lua_pop( m_pLuaState, 1 );

        return nCount;
    }

    /**
     * @brief Give Lua access to the engine's ITileMap/SoundManager/SpritePool
     * and register the primitive API modules (camera, input, tilemap, sound,
     * sprite) that depend on them. Called from EngineHost's constructor
     * body (not it's member-init list) since it needs m_LuaEngine to already
     * exist as a constructed object to call a method on it - pTileMap itself
     * is just the constructor's own parameter, forwarded straight through
     * (EngineHost keeps no ITileMap* member of it's own; it has no
     * remaining C++-side use for one). Also resolves and exposes
     * the running executable's own directory as the Lua global APP_DIR -
     * deciding where resources actually live relative to that (BASE_PATH)
     * is main.lua's own job, not C++'s; the one exception is the bootstrap
     * entry script itself, which EngineHost must locate on disk before any
     * Lua exists to make that decision (see GetApplicationDirectory below).
     *
     * @param pTileMap Pointer to the engine's tile map instance;
     * @param pDrawSurface Pointer to the engine's screen-space drawing
     * surface instance (sunlight v0.12.0's IDrawSurface split - text/
     * rectangle drawing and window-state primitives that don't depend
     * on a loaded map; see LuaEngineUtil::GetDrawSurface's own comment);
     * @param pSoundManager Pointer to the engine's sound manager instance;
     * @param pSpritePool Pointer to the engine's sprite pool instance;
     */
    void LuaEngine :: Init( SunLight :: TileMap :: ITileMap *pTileMap,
                           SunLight :: DrawSurface :: IDrawSurface *pDrawSurface,
                           SunLight :: Sound :: SoundManager *pSoundManager,
                           Engine :: SpritePool *pSpritePool )  {

        std :: lock_guard<std :: mutex>  lock( Engine :: Lua :: LuaEngineUtil :: s_LuaMutex );

        /*
         * IFileSystem::ToVirtualPath() of the real application directory,
         * not that raw real-OS-path value itself - main.cpp mounts that
         * same real directory at this exact virtual-path conversion of
         * itself (not at itself), since a raw Windows path is illegal as
         * PHYSFS_mount()'s own mountPoint argument (see ToVirtualPath's
         * own doc comment, ifilesystem.h). Every APP_DIR-anchored virtual
         * path Lua/EngineHost build from this value must keep matching
         * whatever main.cpp actually mounted things at - both derive from
         * the same GetApplicationDirectory() call and apply the exact same
         * deterministic conversion, so they agree without needing to
         * coordinate explicitly.
         */
        m_strAppDirectory = SunLight :: FileSystem :: IFileSystem :: ToVirtualPath(
                                 SunLight :: Engines :: EngineFactory :: GetEngine().GetApplicationDirectory() );

        lua_pushlightuserdata( m_pLuaState, pTileMap );
        lua_setglobal( m_pLuaState, "tileMapPtr" );

        lua_pushlightuserdata( m_pLuaState, pDrawSurface );
        lua_setglobal( m_pLuaState, "drawSurfacePtr" );

        lua_pushlightuserdata( m_pLuaState, pSoundManager );
        lua_setglobal( m_pLuaState, "soundManagerPtr" );

        lua_pushlightuserdata( m_pLuaState, pSpritePool );
        lua_setglobal( m_pLuaState, "spritePoolPtr" );

        lua_pushstring( m_pLuaState, m_strAppDirectory.c_str() );
        lua_setglobal( m_pLuaState, "APP_DIR" );

        Engine :: Lua :: LuaAppApi :: Register( m_pLuaState );
        Engine :: Lua :: LuaTextApi :: Register( m_pLuaState );
        Engine :: Lua :: LuaCameraApi :: Register( m_pLuaState );
        Engine :: Lua :: LuaInputApi :: Register( m_pLuaState );
        Engine :: Lua :: LuaTilemapApi :: Register( m_pLuaState );
        Engine :: Lua :: LuaSoundApi :: Register( m_pLuaState );
        Engine :: Lua :: LuaSpriteApi :: Register( m_pLuaState );
        Engine :: Lua :: LuaCollisionApi :: Register( m_pLuaState );

        m_pCollisionListener = new Engine :: Lua :: LuaCollisionListener( m_pLuaState );
        pTileMap -> GetCollisionManager().AddCollisionListener( m_pCollisionListener );
    }

    /**
     * @brief Return the running executable's own directory (also exposed to
     * Lua as APP_DIR by Init above) - used by EngineHost to locate the
     * bootstrap entry script on disk before any Lua exists to do so itself.
     *
     * @return The application's own directory (trailing separator
     * included), or an empty string if Init hasn't run yet.
     */
    const std :: string& LuaEngine :: GetApplicationDirectory( void ) const  {

        return m_strAppDirectory;
    }

    /**
     * @brief Constructor. Initialize all class data;
     *
     * @param pScriptProcessor Pointer to an initialized script processor;
     *
     */
    LuaEngine :: LuaEngine( SunLight :: Scripting :: ScriptProcessor *pScriptProcessor )  {

        m_pScriptProcessor  = pScriptProcessor;
        m_pLuaState         = luaL_newstate();
        m_pCollisionListener = nullptr;
        m_Timers.clear();

        if( m_pLuaState )  {
            // load Lua base libraries (print/math/etc)
            luaL_openlibs( m_pLuaState );
            RegisterCalls();
        }
    }

    /**
     * @brief Destructor. Finalize all class data;
     *
     */
    LuaEngine :: ~LuaEngine( void )  {

        for( auto timer : m_Timers )  {
            SunLight :: Concurrent :: Timer *pTimer = timer.second;

            delete pTimer;
        }

        delete m_pCollisionListener;

        if( m_pLuaState )  {
            lua_close( m_pLuaState );
        }
    }

    /**
     * @brief Execute a lua script - this is the ONE entry point that loads
     * the game's very first script (EngineHost::LoadLuaScript), before
     * Lua's own overridden "dofile" global (LuaFileSystemApi::DoFile) even
     * exists to be called. Originally used the C API's luaL_dofile(),
     * which reads via a raw fopen() internally - a real gap found while
     * building Phase 12's archive-distribution support: every other
     * resource load in this engine (dofile, load_json, textures, sound,
     * tilemaps) already routed through SunLight::FileSystem, but this one
     * didn't, so the first script of an archive-distributed game could
     * never even be reached. Reads the file itself via
     * SunLight::FileSystem::FileSystemFactory instead (mirrors
     * LuaFileSystemApi::DoFile's own approach exactly - luaL_loadbuffer +
     * lua_pcall in place of luaL_loadfile's internal fopen), so this now
     * works identically whether strFileName is a real loose-directory path
     * or a virtual path inside a mounted archive.
     *
     * @param strFileName Lua script file name to execute;
     * @return true If operation was succesful;
     * @return false If operation has failed;
     */
    bool LuaEngine :: RunFile( std :: string strFileName )  {

        std :: lock_guard<std :: mutex>  lock( Engine :: Lua :: LuaEngineUtil :: s_LuaMutex );

        if( !m_pLuaState )
            return false;

        std :: vector<unsigned char>  data;

        if( !SunLight :: FileSystem :: FileSystemFactory :: GetFileSystem().ReadFile( strFileName, data ) )  {
            fprintf( stderr, "Lua error: cannot open %s\n", strFileName.c_str() );

            return false;
        }

        std :: string  strChunkName = std :: string( "@" ) + strFileName;
        int            nLoadStatus  = luaL_loadbuffer( m_pLuaState, reinterpret_cast<const char *>( data.data() ), data.size(), strChunkName.c_str() );

        if( nLoadStatus != LUA_OK || lua_pcall( m_pLuaState, 0, LUA_MULTRET, 0 ) )  {
            fprintf( stderr, "Lua error: %s\n", lua_tostring( m_pLuaState, -1 ) );
            lua_pop( m_pLuaState, 1 );

            return false;
        }

        return true;
    }
  }
}
