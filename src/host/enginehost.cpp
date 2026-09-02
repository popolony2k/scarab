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

#include "host/enginehost.h"
#include "lua/luacryptoapi.h"
#include "filesystem/filesystemfactory.h"
#include <chrono>
#include <fstream>
#include <filesystem>
#include <vector>
#include <nlohmann/json.hpp>

using namespace std :: chrono;
namespace fs = std :: filesystem;


/*
 * Internal module definitions
 */
#define __INACTIVE_SPRITE_QUEUE_CHECK_MILLI               2000
#define __FRAME_DELTA_MILLI                               ( 1000 / 60 )

/*
 * The only sound id C++ still has any opinion about - matches the old
 * SoundUniqueId::ID_NO_AUDIO. Every other sound id is opaque to C++ and
 * defined purely in Lua now (src/soundids.lua) - this one
 * isn't, since it's a genuine C++-side sentinel ("no song currently
 * tracked") that nothing in Lua ever references by name.
 */
#define __SOUND_ID_NO_AUDIO                                0

/*
 * There is no compiled-in default entry script anymore - main.cpp requires
 * the caller to name one explicitly (a .lua file, or a .json project file
 * with a "main_script" field), and refuses to start at all otherwise. See
 * ResolveEntryScript below for how the two forms are told apart and
 * resolved. Everything else resources-related (BASE_PATH itself) stays
 * main.lua's own job, computed from the APP_DIR global LuaEngine::Init()
 * exposes - this is only about locating the *first* Lua file to run,
 * before any of that machinery exists yet.
 */


/**
 * @brief All states machine supported by game.
 */
enum __EngineStateHandlers  {
    STATE_FATAL_ERROR_HANDLING = 0,
    STATE_INIT_ENGINE,
    STATE_STAGE_RUNNING
};

/**
 * @brief Directory-of-a-virtual-path, for the .zip branch of @see
 * Scarab::Host::EngineHost::ResolveEntryScript only - a virtual path
 * inside an archive always uses '/' regardless of platform (matches
 * SunLight::FileSystem::IFileSystem's own documented contract), so this
 * deliberately does its own plain string split rather than reusing
 * std::filesystem::path (which would use '\' on Windows, not matching
 * what PhysFS itself expects for a virtual path). The .json/.lua
 * branches keep using std::filesystem::path as before - those really are
 * native OS paths, not virtual ones.
 * @param strVirtualPath A virtual path;
 * @return Everything before the last '/', or "" if there isn't one;
 */
static std :: string GetVirtualParentPath( const std :: string &strVirtualPath )  {

    std :: string :: size_type  nSlash = strVirtualPath.find_last_of( '/' );

    return ( nSlash == std :: string :: npos ) ? std :: string() : strVirtualPath.substr( 0, nSlash );
}

/**
 * @brief Joins a virtual directory and a relative virtual path with '/' -
 * see @see GetVirtualParentPath for why this doesn't use
 * std::filesystem::path. Strips a leading '/' off strRelative first (a
 * user-supplied --entry/-e value is the one place this can come from,
 * eg. "--entry /main.lua") - PhysFS itself already tolerates the
 * resulting redundant "//" without one (confirmed live), but stripping it
 * here keeps the built path itself clean rather than relying on that
 * tolerance.
 */
static std :: string JoinVirtualPath( const std :: string &strDir, const std :: string &strRelative )  {

    std :: string  strCleanRelative = ( !strRelative.empty() && strRelative.front() == '/' ) ? strRelative.substr( 1 ) : strRelative;

    return strDir.empty() ? strCleanRelative : ( strDir + "/" + strCleanRelative );
}

namespace Scarab  {
    namespace Host  {

        /**
         * @brief Load a Lua script. strFileName is used exactly as given
         * (already resolved to a real filesystem path by the caller) -
         * unlike the rest of resources/, this one path isn't relative to
         * APP_DIR, since it's resolved relative to whatever the user
         * actually pointed the program at (see ResolveEntryScript).
         * @param strFileName The already-resolved script file path to load;
         */
        bool EngineHost :: LoadLuaScript( std :: string strFileName ) {

            if( !m_LuaEngine.RunFile( strFileName ) )  {
                OnError( "Error executing stage Lua script => " + strFileName );
                return false;;
            }

            if( !m_ScriptProcessorMachine.Compile() )  {
                OnError( "Error compiling script commands => " + strFileName );
                return false;
            }

            return true;
        }

        /**
         * @brief Resolve the user-provided entry argument (m_strEntryArg,
         * originally the CLI11 "entry" positional in main.cpp) to an
         * actual Lua script path. Three forms are accepted, told apart by
         * extension:
         *   - a .lua path, used directly as the entry script (unchanged
         *     from before Phase 12 - m_strEntryOverride combined with this
         *     form is a usage error, since there's no project-file concept
         *     here for it to override);
         *   - a .json "project file" with a "main_script" field, resolved
         *     relative to the project file's own directory (not APP_DIR -
         *     a project should be relocatable as a whole without the
         *     executable needing to know where it lives). m_strEntryOverride,
         *     if set, names a *different* .json to use instead of
         *     strEntryArg itself - everything else about this form is
         *     otherwise unchanged;
         *   - a .zip archive (Phase 12, archive-distribution): mounted at
         *     LuaEngine::GetApplicationDirectory()'s own value - the exact
         *     same virtual location the loose-directory mount in main.cpp
         *     already uses for APP_DIR - searched first (bAppendToPath=
         *     false), so every already-existing APP_DIR-anchored path this
         *     game's own Lua/JSON already builds (BASE_PATH and everything
         *     under it) resolves into the archive unchanged; nothing
         *     downstream needs to know the difference. m_strEntryOverride,
         *     if set, is the in-archive virtual path to the *actual entry
         *     point* (default: "project.json" at the archive's own root) -
         *     told apart by it's own extension, mirroring exactly how the
         *     top-level entry argument itself is told apart: a ".lua" entry
         *     is used directly as the entry script (no project-file
         *     indirection at all), anything else is read and parsed as a
         *     .json project file with a "main_script" field, same as the
         *     loose-directory .json case. The resolved script path is then
         *     a normal APP_DIR-anchored virtual path, read the same way the
         *     loose-directory case already reads every other resource -
         *     see LoadLuaScript / LuaEngine::RunFile.
         * Anything else (missing/unreadable file, malformed JSON, missing
         * "main_script", a bad --entry/.lua combination) is reported via
         * strOutScriptPath being left untouched and this returning false.
         *
         * @param strEntryArg The raw entry argument (a .lua, .json, or .zip path);
         * @param strOutScriptPath Receives the resolved script path on success;
         * @return true if strEntryArg resolved to a usable script path.
         */
        bool EngineHost :: ResolveEntryScript( const std :: string& strEntryArg, std :: string& strOutScriptPath )  {

            fs :: path   entryPath( strEntryArg );
            std :: string strExtension = entryPath.extension().string();

            if( strExtension == ".zip" )  {
                const std :: string  &strAppDir = m_LuaEngine.GetApplicationDirectory();

                if( !SunLight :: FileSystem :: FileSystemFactory :: GetFileSystem().Mount( strEntryArg, strAppDir, false ) )  {
                    OnError( "Error mounting archive => " + strEntryArg );
                    return false;
                }

                std :: string  strInnerEntry = m_strEntryOverride.empty() ? "project.json" : m_strEntryOverride;
                std :: string  strInnerEntryPath = JoinVirtualPath( strAppDir, strInnerEntry );

                if( fs :: path( strInnerEntry ).extension() == ".lua" )  {
                    strOutScriptPath = strInnerEntryPath;
                    return true;
                }

                std :: string  strEntryJsonPath = strInnerEntryPath;
                std :: vector<unsigned char>  data;

                if( !SunLight :: FileSystem :: FileSystemFactory :: GetFileSystem().ReadFile( strEntryJsonPath, data ) )  {
                    OnError( "Error opening project file inside archive => " + strEntryJsonPath );
                    return false;
                }

                /*
                 * Transparent content-encryption support (checkpoint 3,
                 * docs/content-encryption-plan.md) - a packed, encrypted
                 * .zip's own project.json needs this same hook as every
                 * other read path (LuaEngine::RunFile/LuaJsonApi::LoadJson/
                 * LuaFileSystemApi::DoFile) - this is a fourth, distinct
                 * read site (project.json is read directly here, in C++,
                 * never through load_json), easy to miss for exactly that
                 * reason. Silently falls back to the raw bytes as-is
                 * (plaintext) if decryption doesn't apply.
                 */
                std :: vector<unsigned char>  decryptedProjectData;

                if( Engine :: Lua :: LuaCryptoApi :: TryDecryptBytes( data, decryptedProjectData ) )
                    data = std :: move( decryptedProjectData );

                nlohmann :: json  projectData = nlohmann :: json :: parse( data.begin(), data.end(), nullptr, false );

                if( projectData.is_discarded() || !projectData.contains( "main_script" ) )  {
                    OnError( "Error parsing project file (missing or invalid \"main_script\") => " + strEntryJsonPath );
                    return false;
                }

                strOutScriptPath = JoinVirtualPath( GetVirtualParentPath( strEntryJsonPath ), projectData["main_script"].get<std :: string>() );

                return true;
            }

            if( strExtension != ".json" )  {
                if( !m_strEntryOverride.empty() )  {
                    OnError( "Error: --entry/-e cannot be combined with a direct .lua script entry point => " + strEntryArg );
                    return false;
                }

                strOutScriptPath = strEntryArg;
                return true;
            }

            std :: string  strEffectiveJsonPath = m_strEntryOverride.empty() ? strEntryArg : m_strEntryOverride;
            fs :: path     effectiveEntryPath( strEffectiveJsonPath );

            std :: ifstream projectFile( effectiveEntryPath );

            if( !projectFile.is_open() )  {
                OnError( "Error opening project file => " + strEffectiveJsonPath );
                return false;
            }

            nlohmann :: json  projectData = nlohmann :: json :: parse( projectFile, nullptr, false );

            if( projectData.is_discarded() || !projectData.contains( "main_script" ) )  {
                OnError( "Error parsing project file (missing or invalid \"main_script\") => " + strEffectiveJsonPath );
                return false;
            }

            strOutScriptPath = ( effectiveEntryPath.parent_path() / projectData["main_script"].get<std :: string>() ).string();

            return true;
        }

        /**
         * @brief Execute the main script entry point, resolved from the
         * user-provided m_strEntryArg (a .lua path or .json project file -
         * see ResolveEntryScript). There is no compiled-in default; a
         * failure here (bad path, malformed project file) is a normal
         * fatal-error case, same as any other startup failure.
         *
         * @return true If operation was succesfull;
         * @return false If operation has failed;
         */
        bool EngineHost :: RunLuaScriptMainEntryPoint( void )  {

            std :: string strScriptPath;

            if( !ResolveEntryScript( m_strEntryArg, strScriptPath ) )
                return false;

            return LoadLuaScript( strScriptPath );
        }

        /**
         * @brief Resolve WAIT_SPRITES_QUEUE_EMPTY once every Lua-owned enemy
         * has left the screen. This used to also drain the legacy active/
         * inactive/destroyed sprite queues (stock recycling, blink/explosion
         * timers) - all dead now that every sprite type is Lua/SpritePool-
         * owned, so only the periodic "is the queue actually empty" signal
         * sp_wait_queue_empty() depends on survives here.
         */
        void EngineHost :: CheckSpritesQueueEmpty( void )  {

            uint64_t nTimeMilli = duration_cast<milliseconds>( steady_clock :: now().time_since_epoch() ).count();

            if( nTimeMilli >= m_nClearInactiveSpriteQueueMilli )  {
                if( m_LuaEngine.GetActiveEnemyCount() <= 0 )  {
                    m_ScriptProcessorMachine.ResetWaitSpritesQueueEmptyCmd();
                }

                m_nClearInactiveSpriteQueueMilli = nTimeMilli + __INACTIVE_SPRITE_QUEUE_CHECK_MILLI;
            }
        }

        /**
         * @brief Process all scripts commands previously added to scripting engine machine.
         */
        void EngineHost :: RunScriptMachine( void )  {

            m_ScriptProcessorMachine.Run();

            /*
             * Loop current BGM song when it finishes.
             */
            PlaySound( m_CurrentSong );
        }

        /**
         * @brief Must be implemented to provide sprite sound handling.
         * @param id The sound id to play;
         * @param bIgnorePlaying If set will ignore if player is playing any sound and will
         * play again (cutting current playing).
         */
        bool EngineHost :: PlaySound( uint16_t id, bool bIgnorePlaying )  {

            if( ( id != __SOUND_ID_NO_AUDIO ) && ( !m_SoundManager.IsPlaying( id ) || bIgnorePlaying ) )
                return m_SoundManager.Play( id );

            return true;
        }

        /**
         * @brief Must be implemented to provide sprite sound handling.
         * @param id The sound id to pause;
         */
        bool EngineHost :: PauseSound( uint16_t id )  {

            if( m_SoundManager.IsPlaying( id ) )
                return m_SoundManager.Pause( id );

            return true;
        }

        /**
         * @brief Must be implemented to provide sprite sound handling.
         * @param id The sound id to stop;
         */
        bool EngineHost :: StopSound( uint16_t id )  {

            if( m_SoundManager.IsPlaying( id ) )
                return m_SoundManager.Stop( id );

            return true;
        }

        /**
         * @brief Must be implemented to provide sprite sound handling.
         * @param id The sound id to resume;
         */
        bool EngineHost :: ResumeSound( uint16_t id )  {

            if( !m_SoundManager.IsPlaying( id ) )
                return m_SoundManager.Resume( id );

            return true;
        }

        /**
         * @brief Error handling state.
         * All fatal game error are handled here.
         */
        void EngineHost :: FatalErrorHandler( void )  {

            fprintf( stderr, "[FATAL] - %s\n", m_strLastError.c_str() );
            exit( EXIT_FAILURE );
        }

        /**
         * @brief Initialize engine, loading all shared objects that will be used
         * by game through all states.
         */
        void EngineHost :: InitEngineStateHandler( void )  {

            /*
             * Config loading and stage/sound bootstrap live entirely in Lua
             * now (bootstrap.lua/spriteconfig.lua, dofile'd from main.lua),
             * as does player input handling (player.lua polls
             * input_is_key_down/input_is_gamepad_button_down every
             * on_update frame) - this state handler just needs to run
             * main.lua and let it take over.
             */
            bool     bSuccess = RunLuaScriptMainEntryPoint();

            if( bSuccess )
                m_CurrentStateHandler = m_aEngineStateHandlers[STATE_STAGE_RUNNING];
            else
                m_CurrentStateHandler = m_aEngineStateHandlers[STATE_FATAL_ERROR_HANDLING];
        }

        /**
         * Stage processing state machine handler.
         */
        void EngineHost :: RunStageStateHandler( void )  {

            CheckSpritesQueueEmpty();
            RunScriptMachine();

            m_SpritePool.UpdateAll();
            m_LuaEngine.CallOnUpdate( __FRAME_DELTA_MILLI );
        }

        /**
         * Constructor. Initialize all class data.
         * @param pTileMap Pointer to the @link IWorld object
         * that will be used by this engine;
         * @param strEntryArg The entry argument main.cpp requires on the
         * command line (a .lua script path or a .json project file) -
         * resolved later, from InitEngineStateHandler, since a failure
         * there already has a normal fatal-error path to report through.
         */
        EngineHost :: EngineHost( SunLight :: TileMap :: ITileMap *pTileMap,
                                 SunLight :: DrawSurface :: IDrawSurface *pDrawSurface,
                                 std :: string strEntryArg,
                                 std :: string strEntryOverride )  : m_LuaEngine( &m_ScriptProcessorMachine ) {

            m_CurrentStateHandler   = nullptr;
            m_nClearInactiveSpriteQueueMilli = 0;
            m_strEntryArg      = strEntryArg;
            m_strEntryOverride = strEntryOverride;

            std :: srand( ( unsigned int ) time( NULL ) );

            m_strLastError.clear();

            /*
             * Init() resolves the executable's own directory and exposes it
             * to Lua as APP_DIR - main.lua decides where resources actually
             * live relative to that (BASE_PATH) itself. Neither pTileMap
             * nor pDrawSurface (sunlight v0.12.0's IDrawSurface split - see
             * LuaEngineUtil::GetDrawSurface) is kept as a member - EngineHost
             * has no C++-side use for either anymore now that camera auto-
             * scroll moved to Lua (camera.lua), so both are forwarded
             * straight through instead of stored.
             */
            m_LuaEngine.Init( pTileMap, pDrawSurface, &m_SoundManager, &m_SpritePool );

            // Engine state handlers setup
            m_aEngineStateHandlers[STATE_FATAL_ERROR_HANDLING] = std :: bind( &EngineHost :: FatalErrorHandler, this );
            m_aEngineStateHandlers[STATE_INIT_ENGINE]          = std :: bind( &EngineHost :: InitEngineStateHandler, this );
            m_aEngineStateHandlers[STATE_STAGE_RUNNING]        = std :: bind( &EngineHost :: RunStageStateHandler, this );
            m_CurrentStateHandler = m_aEngineStateHandlers[STATE_INIT_ENGINE];

            /*
             * Collision rule setup used to be a handful of hardcoded
             * AddColliderToColliderRule calls here, keyed off a C++-only
             * layer-id enum specific to one particular game. Moved entirely
             * to Lua (via the already-existing collision_add_rule primitive)
             * - which layers collide with which is a specific game's own
             * design, not engine plumbing, so it belongs in that game's own
             * Lua bootstrap, not here.
             */

            m_ScriptProcessorMachine.AddScriptListener( this );
        }

        /**
         * Destructor. Finalize all class data.
         */
        EngineHost :: ~EngineHost( void )  {

        }

        /**
         * Provide action on each frame update.
         * @param tileMap The @link ITileMap object used to interact with tile map
         * renderer implementation;
         */
        void EngineHost :: OnUpdate( SunLight :: TileMap :: ITileMap& tileMap )  {

            if( m_CurrentStateHandler )
                m_CurrentStateHandler();
        }

        /**
         * @brief Perform world listener cleaning before world window closing.
         * Runs while TileMapRenderer's window/GL context is still alive (see
         * TileMapRenderer::Stop(), which calls every listener's OnStop()
         * before CloseWindow()) - releasing the sprite pool's textures here,
         * rather than leaving it to ~SpritePool() at program exit, avoids
         * unloading GPU textures after the context is already gone.
         */
        void EngineHost :: OnStop( void )  {
            m_SpritePool.Clear();
        }

        /**
         * @brief Must be implemented to respond STOP_SONG_CMD command;
         *
         * @param cmd The command received/
         * @param nEventId The event id;
         */
        void EngineHost :: OnCommand( SunLight :: Scripting :: Commands cmd, uint16_t nEventId )  {

            switch ( cmd )  {

                case SunLight :: Scripting :: Commands :: LOAD_STAGE_CMD :
                    /*
                     * Stage bootstrap (map + stage script loading) lives in
                     * Lua's on_load_stage(stageId) (bootstrap.lua) - see
                     * LuaEngine::TryDispatchLoadStage. Compile() still has to
                     * happen here in C++ (no Lua primitive reaches the
                     * ScriptProcessor's compile step), same as the state
                     * transition below.
                     */
                    if( m_LuaEngine.TryDispatchLoadStage( nEventId ) && m_ScriptProcessorMachine.Compile() )  {
                        m_CurrentStateHandler = m_aEngineStateHandlers[STATE_STAGE_RUNNING];
                    }
                    else  {
                        char szMsg[50];

                        snprintf( szMsg, sizeof( szMsg ), "Error loading stage id [%d]", nEventId );
                        OnError( szMsg );
                    }
                break;

                case  SunLight :: Scripting :: Commands :: MOVE_SPRITES_TO_SCREEN_CMD :
                    /*
                     * Wave-spawn state ids are entirely Lua-owned now (see
                     * src/wavestates.lua and each enemy
                     * module's register_wave_handler calls) - C++ no longer
                     * knows what a valid range even is, so there's nothing
                     * left to bounds-check here; TryDispatchMoveSpritesToScreen's
                     * own return value is the only signal of whether this
                     * id was actually handled.
                     */
                    if( !m_LuaEngine.TryDispatchMoveSpritesToScreen( nEventId ) )  {
                        char szMsg[50];

                        snprintf( szMsg, sizeof( szMsg ), "Unhandled sprite move state [%d]", nEventId );
                        OnError( szMsg );
                    }
                break;

                case  SunLight :: Scripting :: Commands :: PLAY_SONG_CMD :
                    m_CurrentSong = nEventId;
                case  SunLight :: Scripting :: Commands :: PLAY_SONG_DIRECT_CMD :

                    if( !PlaySound( nEventId ) ) {
                        char szMsg[50];

                        snprintf( szMsg, sizeof( szMsg ), "Error playing song id [%d]", nEventId );
                        OnError( szMsg );
                    }
                break;

                case  SunLight :: Scripting :: Commands :: STOP_SONG_CMD :
                case  SunLight :: Scripting :: Commands :: STOP_SONG_DIRECT_CMD :

                    /*
                     * Either form (queued or direct) untracks the BGM once
                     * it's the song actually being stopped - not just the
                     * queued STOP_SONG_CMD as before. Without this, a direct
                     * stop_song() on the currently tracked BGM would go
                     * silent for exactly one frame before RunScriptMachine's
                     * PlaySound(m_CurrentSong) auto-reloop check restarted
                     * it right back - direct calls are the only reliable way
                     * to affect playback once a stage script's own perpetual
                     * sp_goto_label wave loop is running (see gameover.lua's
                     * header comment for why queued sp_* commands enqueued
                     * from live Lua code can go permanently unreached).
                     */
                    if( nEventId == m_CurrentSong )
                        m_CurrentSong = __SOUND_ID_NO_AUDIO;

                    if( !StopSound( nEventId ) ) {
                        char szMsg[50];

                        snprintf( szMsg, sizeof( szMsg ), "Error stopping song id [%d]", nEventId );
                        OnError( szMsg );
                    }
                break;

                case  SunLight :: Scripting :: Commands :: PAUSE_SONG_CMD :
                case  SunLight :: Scripting :: Commands :: PAUSE_SONG_DIRECT_CMD :

                    /*
                     * Untrack only if this id is actually the currently
                     * tracked BGM (same guard STOP_SONG_CMD/STOP_SONG_DIRECT_CMD
                     * already use above) - without this, RunScriptMachine's
                     * own per-frame PlaySound(m_CurrentSong) auto-reloop
                     * check would see !IsPlaying(m_CurrentSong) (true once
                     * paused - raylib's IsSoundPlaying is false while
                     * paused) and immediately restart it from the beginning
                     * the very next frame, making a paused BGM impossible.
                     */
                    if( nEventId == m_CurrentSong )
                        m_CurrentSong = __SOUND_ID_NO_AUDIO;

                    if( !PauseSound( nEventId ) ) {
                        char szMsg[50];

                        snprintf( szMsg, sizeof( szMsg ), "Error pausing song id [%d]", nEventId );
                        OnError( szMsg );
                    }
                break;

                case  SunLight :: Scripting :: Commands :: RESUME_SONG_CMD :
                case  SunLight :: Scripting :: Commands :: RESUME_SONG_DIRECT_CMD :

                    /*
                     * Only re-track if nothing else is currently tracked -
                     * mirrors the untrack above without clobbering some
                     * other song that might legitimately be tracked already.
                     */
                    if( m_CurrentSong == __SOUND_ID_NO_AUDIO )
                        m_CurrentSong = nEventId;

                    if( !ResumeSound( nEventId ) ) {
                        char szMsg[50];

                        snprintf( szMsg, sizeof( szMsg ), "Error resuming song id [%d]", nEventId );
                        OnError( szMsg );
                    }
                break;

            default:
                break;
            }

        }

        /**
         * @brief Runtime error handling.
         *
         * @param strError The engine error message received;
         */
        void EngineHost :: OnError( std :: string strError ) {

            m_strLastError = strError;

            fprintf( stderr, "%s\n", strError.c_str() );
        }
    }
}
