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
         * Deliberately does NOT clear nativeTextureWidth/nativeTextureHeight
         * here - this used to look like an oversight (a fix that did, commit
         * 938f3c7, was landed 2026-08-17), but is actually load-bearing, and
         * the fix was reverted the same day (039741c) once live A/B testing
         * showed it caused real, reproducible sprite corruption (first
         * reported as "Cylinder enemies with cropped sprites", later also
         * seen on the player ship's own explosion sequence). The mechanism
         * is now fully understood, confirmed via live diagnostic logging
         * plus reading sunlight's own source directly (not guessed):
         *
         * 1. `Scarab::Lua::LuaSpriteApi::ConfigureTexture` (src/lua/
         *    luaspriteapi.cpp) reads `TextureCanvas::GetDimension2D()`
         *    right after `Load()` to learn the texture's true native pixel
         *    size (GetOrCacheNativeWidth/Height below exist specifically to
         *    cache that value reliably - see their own comments).
         * 2. `TextureCanvas::Load()` (sunlight's canvas/texturecanvas.cpp)
         *    only writes the freshly-loaded image's real width/height into
         *    that dimension struct if it's currently (0,0) - a guard that's
         *    true the FIRST time a given TextureCanvas C++ object is ever
         *    loaded, and never again afterward.
         * 3. `Sprite::AddTextureSequence` (sunlight's sprite/sprite.cpp),
         *    called once per ConfigureTexture at the end of every
         *    configure, permanently repoints that same canvas's dimension
         *    pointer (`GraphicObject::SetDimension2DPtr`) at the owning
         *    Sprite's OWN shared dimension struct - not the canvas's
         *    private one anymore. That shared struct then gets overwritten
         *    to the sprite's current *tile* size (native width / frame
         *    count) by `SetTileSize`, every configure.
         *
         * SpritePool reuses the same TextureCanvas C++ objects across every
         * Release()/Acquire() cycle (they live in stSlot::textures, never
         * destroyed - only Unload()ed and re-Load()ed). So on a slot's
         * SECOND and every later configure, step 2's guard is false (the
         * dimension struct is already aliased to the sprite's shared one,
         * left at the PREVIOUS life's tile size, not (0,0)) - Load() skips
         * writing the real size, and GetDimension2D() silently returns the
         * stale tile size instead. Confirmed live via diagnostic logging
         * across every enemy type (not just Cylinder): a slot's second
         * configure always reports its own FIRST configure's tile size as
         * the "native" width - e.g. Cylinder's real 128px native width
         * comes back as 32 (=128/4 frames) on every recycle, Satellite's
         * 96 comes back as 32 (=96/3), Galileo's 192 comes back as 24
         * (=192/8) - then that wrong value gets divided by frame count
         * AGAIN in ConfigureTexture, producing a badly undersized (cropped)
         * sprite.
         *
         * GetOrCacheNativeWidth/Height's job is exactly to prevent this -
         * cache the FIRST (genuinely reliable) native size forever and
         * ignore whatever a later reconfigure's now-aliased GetDimension2D()
         * reports. Clearing that cache in Release() (938f3c7) throws this
         * protection away on every single recycle, not just the narrower
         * cross-texture-size sharing case (LASER/SPREAD/BASE briefly
         * sharing one pool, see GetOrCacheNativeWidth's own comment) it was
         * actually written for - reproducing the exact "stale/aliased
         * width" symptom the cache exists to prevent, just far more often.
         * This is a genuine sunlight behavior (TextureCanvas::Load()'s
         * write-once guard plus Sprite::AddTextureSequence's dimension
         * aliasing), not a sunlight bug to fix - a Sprite's sequences
         * sharing one on-screen dimension is reasonable by design. This
         * cache is the correct, permanent place to work around it. Do not
         * reinstate clearing here without giving every recycled slot's
         * TextureCanvas its own always-private dimension storage instead
         * (a real sunlight-side change, not a one-line SpritePool fix).
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
         * reported dimension can no longer be trusted (@see stSlot::nativeTextureSizes,
         * and Release()'s own comment above for the fully-confirmed mechanism).
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

            stNativeTextureSize&  size = slot.nativeTextureSizes[nSequenceId];

            if( size.nWidth.has_value() )
                return size.nWidth.value();

            size.nWidth = nCurrentWidth;

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

            stNativeTextureSize&  size = slot.nativeTextureSizes[nSequenceId];

            if( size.nHeight.has_value() )
                return size.nHeight.value();

            size.nHeight = nCurrentHeight;

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
