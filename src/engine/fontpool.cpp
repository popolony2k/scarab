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

#include "engine/fontpool.h"

#define __FONT_HANDLE_INDEX_MASK       0xFFFFu   // low 16 bits: the 1-based slot index
#define __FONT_HANDLE_GENERATION_SHIFT 16        // high 16 bits: the slot's generation


namespace Scarab  {
    namespace Engine  {

        /**
         * @brief Pack a slot index and its generation into a font handle (the same packing as SpriteHandle).
         * @param nIndex The slot's 0-based index (stored + 1, so a handle is never 0);
         * @param nGeneration The slot's current generation;
         * @return The handle;
         */
        static FontHandle MakeFontHandle( uint32_t nIndex, uint16_t nGeneration )  {

            return ( ( ( FontHandle ) nGeneration ) << __FONT_HANDLE_GENERATION_SHIFT ) | ( ( nIndex + 1 ) & __FONT_HANDLE_INDEX_MASK );
        }

        /**
         * @brief The slot index a font handle addresses.
         * @param hFont The handle;
         * @return The 0-based slot index; -1 for INVALID_FONT_HANDLE;
         */
        static int32_t FontHandleIndex( FontHandle hFont )  {

            return ( int32_t ) ( hFont & __FONT_HANDLE_INDEX_MASK ) - 1;
        }

        /**
         * @brief The generation a font handle was made with.
         * @param hFont The handle;
         * @return The generation;
         */
        static uint16_t FontHandleGeneration( FontHandle hFont )  {

            return ( uint16_t ) ( hFont >> __FONT_HANDLE_GENERATION_SHIFT );
        }

        /**
         * @brief Keep a loaded font and give it a handle.
         * @param pFont The font, as IDrawSurface::LoadFont gave it;
         * @return Its handle, or INVALID_FONT_HANDLE for no font or a full pool (65535 live fonts);
         */
        FontHandle FontPool :: Add( std :: shared_ptr<SunLight :: Font :: IFont> pFont )  {

            uint32_t  nIndex = 0;

            if( !pFont )
                return INVALID_FONT_HANDLE;

            if( !m_FreeSlots.empty() )  {
                nIndex = m_FreeSlots.back();
                m_FreeSlots.pop_back();
            }
            else  {
                if( m_Slots.size() >= __FONT_HANDLE_INDEX_MASK )
                    return INVALID_FONT_HANDLE;

                nIndex = ( uint32_t ) m_Slots.size();
                m_Slots.push_back( stSlot() );
            }

            stSlot&  slot = m_Slots[nIndex];

            slot.pFont  = std :: move( pFont );
            slot.bInUse = true;

            return MakeFontHandle( nIndex, slot.nGeneration );
        }

        /**
         * @brief Let go of a handle. Only the pool's reference is dropped: a font still active or
         * registered for a text-object family keeps working until its last owner lets go.
         * @return false for a handle that isn't live (never given out, or already released);
         */
        bool FontPool :: Release( FontHandle hFont )  {

            int32_t  nIndex = FontHandleIndex( hFont );

            if( ( nIndex < 0 ) || ( ( uint32_t ) nIndex >= m_Slots.size() ) )
                return false;

            stSlot&  slot = m_Slots[nIndex];

            if( !slot.bInUse || ( slot.nGeneration != FontHandleGeneration( hFont ) ) )
                return false;

            slot.pFont.reset();
            slot.bInUse = false;
            slot.nGeneration++;      // a stale copy of this handle never resolves again
            m_FreeSlots.push_back( ( uint32_t ) nIndex );

            return true;
        }

        /** @brief The font behind a live handle, or nullptr. */
        std :: shared_ptr<SunLight :: Font :: IFont> FontPool :: Resolve( FontHandle hFont ) const  {

            int32_t  nIndex = FontHandleIndex( hFont );

            if( ( nIndex < 0 ) || ( ( uint32_t ) nIndex >= m_Slots.size() ) )
                return nullptr;

            const stSlot&  slot = m_Slots[nIndex];

            if( !slot.bInUse || ( slot.nGeneration != FontHandleGeneration( hFont ) ) )
                return nullptr;

            return slot.pFont;
        }

        /** @brief Let go of every font - call while the window is still open (EngineHost::OnStop). */
        void FontPool :: Clear( void )  {

            m_Slots.clear();
            m_FreeSlots.clear();
        }
    }
}
