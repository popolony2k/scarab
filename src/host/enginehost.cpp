/*
 * enginehost.cpp
 *
 *  Created on: Sep 9, 2021
 *      Author: popolony2k
 */

#include "host/enginehost.h"
#include <chrono>
#include <fstream>
#include <filesystem>
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
 * defined purely in Lua now (resources/scripts/soundids.lua) - this one
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
         * originally argv[1]) to an actual Lua script path. Two forms are
         * accepted, told apart by extension:
         *   - a .lua path, used directly as the entry script;
         *   - a .json "project file" with a "main_script" field, resolved
         *     relative to the project file's own directory (not APP_DIR -
         *     a project should be relocatable as a whole without the
         *     executable needing to know where it lives).
         * Anything else (missing file, malformed JSON, missing
         * "main_script") is reported via strOutScriptPath being left
         * untouched and this returning false.
         *
         * @param strEntryArg The raw entry argument (a .lua or .json path);
         * @param strOutScriptPath Receives the resolved script path on success;
         * @return true if strEntryArg resolved to a usable script path.
         */
        bool EngineHost :: ResolveEntryScript( const std :: string& strEntryArg, std :: string& strOutScriptPath )  {

            fs :: path entryPath( strEntryArg );

            if( entryPath.extension() != ".json" )  {
                strOutScriptPath = strEntryArg;
                return true;
            }

            std :: ifstream projectFile( entryPath );

            if( !projectFile.is_open() )  {
                OnError( "Error opening project file => " + strEntryArg );
                return false;
            }

            nlohmann :: json  projectData = nlohmann :: json :: parse( projectFile, nullptr, false );

            if( projectData.is_discarded() || !projectData.contains( "main_script" ) )  {
                OnError( "Error parsing project file (missing or invalid \"main_script\") => " + strEntryArg );
                return false;
            }

            strOutScriptPath = ( entryPath.parent_path() / projectData["main_script"].get<std :: string>() ).string();

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
                                 std :: string strEntryArg )  : m_LuaEngine( &m_ScriptProcessorMachine ) {

            m_CurrentStateHandler   = nullptr;
            m_nClearInactiveSpriteQueueMilli = 0;
            m_strEntryArg = strEntryArg;

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
             * Collision rule setup (player-vs-enemies, player-vs-enemy-
             * bullets, player-bullets-vs-enemies) used to be 3 hardcoded
             * AddColliderToColliderRule calls here, keyed off the C++-only
             * __SpriteLayerId enum. Moved to Lua (Bootstrap.setup_collision_
             * rules, resources/scripts/bootstrap.lua/layerids.lua) via the
             * already-existing collision_add_rule primitive - which layers
             * collide with which is Caravellius game design, not engine
             * plumbing, so it belongs in Lua like every other enemy-specific
             * decision.
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
                     * resources/scripts/wavestates.lua and each enemy
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
