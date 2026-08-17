/*
 * spritepool.cpp
 *
 *  Created on: Jul 6, 2026
 *      Author: popolony2k
 */

#include "engine/spritepool.h"


namespace Scarab  {
    namespace Engine  {

        /**
         * @brief Constructor. Initialize all class data.
         */
        SpritePool :: SpritePool( void )  {

        }

        /**
         * @brief Destructor. Finalize all class data.
         */
        SpritePool :: ~SpritePool( void )  {

        }

        /**
         * @brief Reserve nCapacity fixed slots for the given type tag. Must be
         * called before Acquire() is used for that tag; a second call for a tag
         * already registered is a no-op returning false.
         *
         * @param strTypeTag The caller-defined type tag (opaque to the pool);
         * @param nCapacity Fixed number of slots to reserve for this type;
         */
        bool SpritePool :: RegisterType( const std :: string &strTypeTag, uint32_t nCapacity )  {

            if( m_FreeListsByType.find( strTypeTag ) != m_FreeListsByType.end() )
                return false;

            __FreeList  freeList;
            uint32_t    nBaseIndex = ( uint32_t ) m_Slots.size();

            for( uint32_t nCount = 0; nCount < nCapacity; nCount++ )  {
                /*
                 * Sprite (and hence stSlot) is neither copyable nor movable, so
                 * the slot must be constructed directly in it's final deque
                 * storage rather than built as a temporary and moved/copied in.
                 */
                m_Slots.emplace_back();

                stSlot&  slot = m_Slots.back();

                slot.strTypeTag  = strTypeTag;
                slot.nGeneration = 1;
                slot.bInUse      = false;

                freeList.push_back( nBaseIndex + nCount );
            }

            m_FreeListsByType.insert( std :: make_pair( strTypeTag, freeList ) );

            return true;
        }

        /**
         * @brief Acquire a free slot for the given type tag, returning an opaque
         * handle.
         *
         * @param strTypeTag The type tag to acquire a slot from;
         * @return A valid handle on success, INVALID_SPRITE_HANDLE if the type
         * wasn't registered or has no free slots left;
         */
        SpriteHandle SpritePool :: Acquire( const std :: string &strTypeTag )  {

            auto  itFreeList = m_FreeListsByType.find( strTypeTag );

            if( ( itFreeList == m_FreeListsByType.end() ) || itFreeList -> second.empty() )
                return INVALID_SPRITE_HANDLE;

            uint32_t  nIndex = itFreeList -> second.front();

            itFreeList -> second.pop_front();

            stSlot&  slot = m_Slots[nIndex];

            slot.bInUse = true;

            return MakeSpriteHandle( nIndex, slot.nGeneration );
        }

        /**
         * @brief Release a previously acquired handle back to it's type's free
         * list. Bumps the slot's generation so stale handles referencing this
         * slot become unresolvable.
         *
         * Does NOT clear nativeTextureWidth/nativeTextureHeight here - a fix
         * that did (commit 938f3c7, landed and reverted the same day,
         * 2026-08-17) was live A/B tested by the user against a real, reproducible
         * player-ship-explosion sprite corruption bug and confirmed to be
         * the cause: fix in -> corruption; fix reverted -> clean. The
         * mechanism was never confirmed - the player ship's own handle is
         * never released for the whole process lifetime (grepped every
         * caller, confirmed), so Release() never even runs for it's slot,
         * and no other consumer of these two caches was found that should
         * be affected either. Left reverted rather than re-landed once
         * "explained" is found later - the original bug that fix targeted
         * (see GetOrCacheNativeWidth/Height's own comments) is already
         * fully solved independently, by games/caravellius/resources/
         * scripts/core/weapon.lua giving LASER/SPREAD/HOMING their own
         * dedicated pools rather than sharing one across differently-sized
         * textures - that workaround doesn't depend on this function at
         * all, so nothing live actually needs this fix reinstated. Worth
         * real investigation before ever re-attempting it, not just
         * reasoning from first principles again.
         *
         * @param handle The handle to release;
         */
        bool SpritePool :: Release( SpriteHandle handle )  {

            int32_t  nIndex = SpriteHandleIndex( handle );

            if( ( nIndex < 0 ) || ( ( uint32_t ) nIndex >= m_Slots.size() ) )
                return false;

            stSlot&  slot = m_Slots[nIndex];

            if( !slot.bInUse || ( slot.nGeneration != SpriteHandleGeneration( handle ) ) )
                return false;

            slot.bInUse = false;
            slot.sprite.SetVisible( false );
            slot.nGeneration++;

            m_FreeListsByType[slot.strTypeTag].push_back( ( uint32_t ) nIndex );

            return true;
        }

        /**
         * @brief Resolve a handle to it's live Sprite object.
         *
         * @param handle The handle to resolve;
         * @return Pointer to the sprite, or nullptr if the handle is invalid,
         * stale, or out of range;
         */
        SunLight :: Sprite :: Sprite* SpritePool :: Resolve( SpriteHandle handle )  {

            int32_t  nIndex = SpriteHandleIndex( handle );

            if( ( nIndex < 0 ) || ( ( uint32_t ) nIndex >= m_Slots.size() ) )
                return nullptr;

            stSlot&  slot = m_Slots[nIndex];

            if( !slot.bInUse || ( slot.nGeneration != SpriteHandleGeneration( handle ) ) )
                return nullptr;

            return &slot.sprite;
        }

        /**
         * @brief Get a handle's texture canvas for the given sequence id, if it
         * was previously created via GetOrCreateTexture().
         *
         * @param handle The sprite handle;
         * @param nSequenceId The texture sequence id;
         * @return Pointer to the texture canvas, or nullptr if the handle is
         * invalid/stale or the sequence hasn't been created yet;
         */
        SunLight :: Canvas :: TextureCanvas* SpritePool :: GetTexture( SpriteHandle handle, int nSequenceId )  {

            int32_t  nIndex = SpriteHandleIndex( handle );

            if( ( nIndex < 0 ) || ( ( uint32_t ) nIndex >= m_Slots.size() ) )
                return nullptr;

            stSlot&  slot = m_Slots[nIndex];

            if( !slot.bInUse || ( slot.nGeneration != SpriteHandleGeneration( handle ) ) )
                return nullptr;

            auto  itTexture = slot.textures.find( nSequenceId );

            return ( itTexture != slot.textures.end() ? &itTexture -> second : nullptr );
        }

        /**
         * @brief Get (creating if necessary) a handle's texture canvas for the
         * given sequence id. The returned pointer's address is stable for the
         * lifetime of the pool (std::map node storage), safe to hand to
         * Sprite::AddTextureSequence().
         *
         * @param handle The sprite handle;
         * @param nSequenceId The texture sequence id;
         * @return Pointer to the texture canvas, or nullptr if the handle is
         * invalid or stale;
         */
        SunLight :: Canvas :: TextureCanvas* SpritePool :: GetOrCreateTexture( SpriteHandle handle, int nSequenceId )  {

            int32_t  nIndex = SpriteHandleIndex( handle );

            if( ( nIndex < 0 ) || ( ( uint32_t ) nIndex >= m_Slots.size() ) )
                return nullptr;

            stSlot&  slot = m_Slots[nIndex];

            if( !slot.bInUse || ( slot.nGeneration != SpriteHandleGeneration( handle ) ) )
                return nullptr;

            return &slot.textures[nSequenceId];
        }

        /**
         * @brief Get a sequence's cached natural (un-frame-divided) pixel width,
         * caching nCurrentWidth as that value the first time this sequence is
         * seen for this handle. Callers must pass the freshly-loaded texture's
         * reported width on every call - it's only actually used (and cached)
         * the first time; every later call returns the cached value regardless
         * of what's passed, since a reconfigured (recycled) texture's own
         * reported dimension can no longer be trusted (@see stSlot::nativeTextureWidth).
         *
         * @param handle The sprite handle;
         * @param nSequenceId The texture sequence id;
         * @param nCurrentWidth The just-loaded texture's currently reported width;
         * @return The cached natural width, or nCurrentWidth if the handle is invalid/stale;
         */
        int SpritePool :: GetOrCacheNativeWidth( SpriteHandle handle, int nSequenceId, int nCurrentWidth )  {

            int32_t  nIndex = SpriteHandleIndex( handle );

            if( ( nIndex < 0 ) || ( ( uint32_t ) nIndex >= m_Slots.size() ) )
                return nCurrentWidth;

            stSlot&  slot = m_Slots[nIndex];

            if( !slot.bInUse || ( slot.nGeneration != SpriteHandleGeneration( handle ) ) )
                return nCurrentWidth;

            auto  itWidth = slot.nativeTextureWidth.find( nSequenceId );

            if( itWidth != slot.nativeTextureWidth.end() )
                return itWidth -> second;

            slot.nativeTextureWidth.insert( std :: make_pair( nSequenceId, nCurrentWidth ) );

            return nCurrentWidth;
        }

        /**
         * @brief Height counterpart of GetOrCacheNativeWidth - same reasoning,
         * since a texture's own natural dimension becomes unreliable on any
         * reconfigure after the first once Sprite::AddTextureSequence has
         * aliased it to the sprite.
         */
        int SpritePool :: GetOrCacheNativeHeight( SpriteHandle handle, int nSequenceId, int nCurrentHeight )  {

            int32_t  nIndex = SpriteHandleIndex( handle );

            if( ( nIndex < 0 ) || ( ( uint32_t ) nIndex >= m_Slots.size() ) )
                return nCurrentHeight;

            stSlot&  slot = m_Slots[nIndex];

            if( !slot.bInUse || ( slot.nGeneration != SpriteHandleGeneration( handle ) ) )
                return nCurrentHeight;

            auto  itHeight = slot.nativeTextureHeight.find( nSequenceId );

            if( itHeight != slot.nativeTextureHeight.end() )
                return itHeight -> second;

            slot.nativeTextureHeight.insert( std :: make_pair( nSequenceId, nCurrentHeight ) );

            return nCurrentHeight;
        }

        /**
         * @brief Advance the animation frame of every in-use sprite. Called once
         * per frame by the engine so Lua never needs to remember to do it.
         */
        void SpritePool :: UpdateAll( void )  {

            for( stSlot &slot : m_Slots )  {
                if( slot.bInUse )
                    slot.sprite.Update();
            }
        }

        /**
         * @brief Release every slot's textures now, while the caller still has a
         * live rendering context. Must run before the window/GL context is torn
         * down (@see EngineHost::OnStop) - TextureCanvas::~TextureCanvas() calls
         * into raylib to unload GPU textures, which segfaults if that happens
         * later, during the pool's own destructor at program exit, after
         * TileMapRenderer::Stop() has already closed the window.
         *
         * Unloads each sprite's textures in place rather than destroying the
         * slots themselves (previously via m_Slots.clear()). Sprites still
         * added to a TileMapRenderer layer at exit (the player ship, any
         * on-screen enemy/bullet) are aliased by raw pointer into these slots
         * via AddSprite - TileMapRenderer::Stop() unloads its own tracked
         * sprites too (via UnloadSprites(), called from UnloadMap(), still
         * before CloseWindow()), and destroying the slots here first left it
         * touching already-freed memory. Sprite::Unload()/TextureCanvas::
         * Unload() are both idempotent (guarded by a null texture-handle
         * check), so leaving the slots alive and merely already-unloaded makes
         * that second pass a safe no-op instead of a use-after-free. The slots
         * themselves are destroyed later, safely, whenever SpritePool's own
         * destructor naturally runs at process exit - by then every texture is
         * already unloaded, so that destruction touches no GL/GPU state.
         */
        void SpritePool :: Clear( void )  {

            for( stSlot &slot : m_Slots )
                slot.sprite.Unload();

            m_FreeListsByType.clear();
        }
    }
}
