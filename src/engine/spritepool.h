/*
 * spritepool.h
 *
 *  Created on: Jul 6, 2026
 *      Author: popolony2k
 */

#ifndef __SPRITEPOOL_H__
#define __SPRITEPOOL_H__

#include <deque>
#include <map>
#include <optional>
#include <string>
#include <unordered_map>
#include "engine/spritehandle.h"
#include "sprite/sprite.h"
#include "canvas/texturecanvas.h"


namespace Scarab  {
    namespace Engine  {

        /**
         * @brief Generic, game-agnostic fixed-capacity sprite pool. Sprite types
         * are registered up-front with a fixed capacity (@see RegisterType);
         * acquiring/releasing a sprite afterwards just recycles a pre-allocated
         * slot - no allocation happens on the per-frame hot path. Has no
         * knowledge of any particular game's sprite types, movement, or
         * lifecycle policy - all of that is Lua's responsibility.
         */
        class SpritePool  {

            /**
             * @brief A sequence's cached native (un-frame-divided) pixel size -
             * width and height are always looked up/cached together (one entry
             * per sequence id, @see stSlot::nativeTextureSizes), so they're kept
             * as a single struct rather than two parallel maps. Each axis is
             * it's own std::optional (not a plain int) so "not cached yet" stays
             * distinguishable from "cached as 0" without a separate presence
             * flag - GetOrCacheNativeWidth/Height each populate only their own
             * axis, independently, so one axis can legitimately be cached before
             * the other.
             */
            struct stNativeTextureSize  {
                std :: optional<int>  nWidth;
                std :: optional<int>  nHeight;
            };

            /**
             * @brief One pool slot: a renderable sprite plus the texture canvases
             * backing each of it's texture sequences (kept in a std::map so
             * their addresses stay stable across insertions - Sprite::
             * AddTextureSequence stores a raw pointer to them).
             */
            struct stSlot  {
                SunLight :: Sprite :: Sprite                          sprite;
                std :: map<int, SunLight :: Canvas :: TextureCanvas>  textures;
                /*
                 * Sprite::AddTextureSequence unconditionally aliases a
                 * TextureCanvas's own dimension to the owning Sprite's shared
                 * dimension the first time it's called for that texture object
                 * - fine for a fresh sprite, but means a reconfigured (recycled)
                 * texture's GetDimension2D() no longer reflects it's own natural
                 * pixel size, it reflects whatever the sprite's size happened to
                 * be left at. Cache each sequence's true natural size the first
                 * time it's seen (while still reliable) so later reconfigures on
                 * a recycled slot don't compute a frame width from stale/aliased
                 * data.
                 */
                std :: map<int, stNativeTextureSize>                  nativeTextureSizes;
                std :: string                                         strTypeTag;
                uint16_t                                              nGeneration;
                bool                                                   bInUse;
            };

            typedef std :: deque<uint32_t>  __FreeList;

            /*
             * std::deque (not std::vector) so growing the pool never invalidates
             * references/pointers into slots already handed out.
             */
            std :: deque<stSlot>                              m_Slots;
            std :: unordered_map<std :: string, __FreeList>   m_FreeListsByType;

            public:

            SpritePool( void );
            ~SpritePool( void );

            bool RegisterType( const std :: string &strTypeTag, uint32_t nCapacity );
            SpriteHandle Acquire( const std :: string &strTypeTag );
            bool Release( SpriteHandle handle );
            SunLight :: Sprite :: Sprite* Resolve( SpriteHandle handle );
            SunLight :: Canvas :: TextureCanvas* GetTexture( SpriteHandle handle, int nSequenceId );
            SunLight :: Canvas :: TextureCanvas* GetOrCreateTexture( SpriteHandle handle, int nSequenceId );
            int GetOrCacheNativeWidth( SpriteHandle handle, int nSequenceId, int nCurrentWidth );
            int GetOrCacheNativeHeight( SpriteHandle handle, int nSequenceId, int nCurrentHeight );
            void UpdateAll( void );
            void Clear( void );
        };
    }
}

#endif  /* __SPRITEPOOL_H__ */
