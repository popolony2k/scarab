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

#ifndef __FONTPOOL_H__
#define __FONTPOOL_H__

#include <cstdint>
#include <memory>
#include <vector>
#include "font/ifont.h"


namespace Scarab  {
    namespace Engine  {

        /**
         * @brief Opaque handle to a font pool slot - the same packing as SpriteHandle: a slot
         * index (low 16 bits, 1-based so 0 stays "invalid") and a generation (high 16 bits)
         * bumped every time the slot is released, so a stale handle fails to resolve instead of
         * addressing whatever font reuses the slot.
         */
        typedef uint32_t FontHandle;

        const FontHandle INVALID_FONT_HANDLE = 0;

        /**
         * @brief The fonts Lua loaded with load_font (plans/MASTER_PLAN.md item C6), one shared
         * sunlight font per handle. Lua-agnostic on purpose, like SpritePool: no lua_State here.
         *
         * A slot holds a std::shared_ptr<IFont> - one owner among several (the engine while the
         * font is active, each text-object family it is registered for). Releasing a handle drops
         * only the pool's reference, so a font that is still active or registered keeps working;
         * the last owner releases it. Clear() must run while the window is open (EngineHost::OnStop,
         * the same rule as SpritePool::Clear), although sunlight's fonts are safe either way: once
         * the window closes, a live font just becomes invalid.
         */
        class FontPool  {

            struct stSlot  {
                std :: shared_ptr<SunLight :: Font :: IFont>  pFont;
                uint16_t                                      nGeneration = 0;
                bool                                          bInUse      = false;
            };

            std :: vector<stSlot>     m_Slots;
            std :: vector<uint32_t>   m_FreeSlots;

            public:

            FontHandle Add( std :: shared_ptr<SunLight :: Font :: IFont> pFont );
            bool Release( FontHandle handle );
            std :: shared_ptr<SunLight :: Font :: IFont> Resolve( FontHandle handle ) const;
            void Clear( void );
        };
    }
}

#endif /* __FONTPOOL_H__ */
