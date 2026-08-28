/*
 * enginehost.h
 *
 *  Created on: Sep 9, 2021
 *      Author: popolony2k
 */

#ifndef __ENGINEHOST_H__
#define __ENGINEHOST_H__

#include <string>
#include <array>
#include <functional>
#include <cstdint>
#include "tilemap/itilemaplistener.h"
#include "drawsurface/idrawsurface.h"
#include "scripting/scriptprocessor.h"
#include "sound/soundmanager.h"
#include "lua/luaengine.h"
#include "engine/spritepool.h"


/*
 * Module definition
 */
#define MAX_STATE_HANDLERS             255


namespace Scarab  {
    namespace Host  {
        /**
         * @brief Scarab's own game engine: the state machine (init/
         * run-stage/fatal-error handlers), the ScriptProcessor/LuaEngine
         * wiring, and the small amount of per-frame bookkeeping (BGM
         * looping, sp_wait_queue_empty resolution) that isn't Lua's
         * job. Used to be split across WorldBase (a reusable base class)
         * and WorldEngine (Scarab's state machine subclassing it) -
         * merged since nothing else ever subclassed WorldBase and the split
         * no longer served polymorphism, just indirection. Renamed from
         * WorldEngine/Caravellius::World once the Lua refactor left it
         * owning no "world" state at all (no tile map pointer, no sprite
         * queues, no camera) - it's a lifecycle/Lua-bridge host, not a
         * world model.
         */
        class EngineHost : public SunLight :: TileMap :: ITileMapListener,
                            public SunLight :: Scripting :: IScriptListener  {

            typedef std :: function<void( void )> FUNCTION_STATE_HANDLER;
            typedef std :: array<FUNCTION_STATE_HANDLER, MAX_STATE_HANDLERS> FunctionStateHandlers;

            uint16_t                                   m_CurrentSong;
            uint64_t                                   m_nClearInactiveSpriteQueueMilli;
            FUNCTION_STATE_HANDLER                     m_CurrentStateHandler;
            SunLight :: Scripting :: ScriptProcessor   m_ScriptProcessorMachine;
            SunLight :: Sound :: SoundManager          m_SoundManager;
            Scarab :: Lua :: LuaEngine                 m_LuaEngine;
            Engine :: SpritePool                       m_SpritePool;

            std :: string                              m_strEntryArg;
            std :: string                              m_strEntryOverride;
            std :: string                              m_strLastError;
            FunctionStateHandlers                      m_aEngineStateHandlers;

            // Queue management update handlers
            void CheckSpritesQueueEmpty( void );

            // Script processing routines
            void RunScriptMachine( void );

            // Sound support
            bool PlaySound( uint16_t id, bool bIgnorePlaying = false );
            bool PauseSound( uint16_t id );
            bool StopSound( uint16_t id );
            bool ResumeSound( uint16_t id );

            // Script management routines
            bool LoadLuaScript( std :: string strFileName );
            bool RunLuaScriptMainEntryPoint( void );
            bool ResolveEntryScript( const std :: string& strEntryArg, std :: string& strOutScriptPath );

            // Machine state handlers
            void FatalErrorHandler( void );
            void InitEngineStateHandler( void );
            void RunStageStateHandler( void );

            public:

            EngineHost( SunLight :: TileMap :: ITileMap *pTileMap,
                       SunLight :: DrawSurface :: IDrawSurface *pDrawSurface,
                       std :: string strEntryArg,
                       std :: string strEntryOverride = std :: string() );
            virtual ~EngineHost( void );

            // World listener event handler implementation
            void OnUpdate( SunLight :: TileMap :: ITileMap& tileMap );
            void OnStop( void );

            void OnCommand( SunLight :: Scripting :: Commands cmd, uint16_t nEventId );
            void OnError( std :: string strError );
        };
    }
}

#endif /* __ENGINEHOST_H__ */
