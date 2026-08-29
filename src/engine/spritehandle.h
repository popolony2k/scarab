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

#ifndef __SPRITEHANDLE_H__
#define __SPRITEHANDLE_H__

#include <cstdint>


namespace Scarab  {
    namespace Engine  {

        /**
         * @brief Opaque handle to a sprite pool slot. Packs a slot index (low 16
         * bits, 1-based so 0 stays reserved for "invalid") and a generation
         * counter (high 16 bits) that's bumped every time a slot is released, so
         * a stale handle referencing an already-recycled slot fails to resolve
         * instead of silently addressing the wrong sprite.
         */
        typedef uint32_t SpriteHandle;

        const SpriteHandle INVALID_SPRITE_HANDLE = 0;

        inline SpriteHandle MakeSpriteHandle( uint32_t nIndex, uint16_t nGeneration )  {

            return ( ( ( SpriteHandle ) nGeneration ) << 16 ) | ( ( nIndex + 1 ) & 0xFFFF );
        }

        inline int32_t SpriteHandleIndex( SpriteHandle handle )  {

            return ( int32_t ) ( handle & 0xFFFF ) - 1;
        }

        inline uint16_t SpriteHandleGeneration( SpriteHandle handle )  {

            return ( uint16_t ) ( handle >> 16 );
        }

        /*
         * A SpriteHandle is stashed inside a Collider's opaque void* data (via
         * SetPtrData/GetPtrData) so a collision callback can identify which
         * sprite collided without exposing raw C++ pointers to Lua. The upper
         * 32 bits of the packed value below are set to a fixed tag no real
         * 64-bit heap pointer will ever carry - originally so callbacks could
         * tell a packed handle apart from a legacy Caravellius stSpriteData*
         * stored in that same slot during the Lua migration (that legacy path
         * is fully gone now, Phase 9, but the tag remains a cheap defensive
         * check against any future non-handle value ending up there).
         */
        const uintptr_t SPRITE_HANDLE_PTR_TAG = 0xDEADBEEF00000000ULL;

        inline void* PackSpriteHandleAsPtr( SpriteHandle handle )  {

            return ( void * ) ( SPRITE_HANDLE_PTR_TAG | ( uintptr_t ) handle );
        }

        inline bool IsPackedSpriteHandle( void *pPtr )  {

            return ( ( ( uintptr_t ) pPtr ) & 0xFFFFFFFF00000000ULL ) == SPRITE_HANDLE_PTR_TAG;
        }

        inline SpriteHandle UnpackSpriteHandleFromPtr( void *pPtr )  {

            return ( SpriteHandle ) ( ( uintptr_t ) pPtr & 0xFFFFFFFFULL );
        }
    }
}

#endif  /* __SPRITEHANDLE_H__ */
