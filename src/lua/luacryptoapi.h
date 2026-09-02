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

#ifndef __LUACRYPTOAPI_H__
#define __LUACRYPTOAPI_H__

extern "C"
{
  #include "lua.h"
}


namespace Scarab  {
    namespace Engine  {
        namespace Lua  {
            /**
             * @brief Content encryption - protects a game's own .zip
             * bundle content so only that game's own privately-built
             * scarab executable can read it back. See
             * docs/content-encryption-plan.md for the full design.
             *
             * @luacategory{Crypto}
             * @luadoc
             * Encrypts/decrypts arbitrary binary data using
             * `SCARAB_CONTENT_KEY` - a shared secret compiled into this
             * specific build of `scarab` (a CMake option, never a Lua
             * value), the same way `SCARAB_VERSION` is. Neither
             * primitive here ever takes a key argument, and the key
             * itself is never exposed to Lua as a readable value -
             * every game's own privately-built `scarab` has it's own
             * key, and only that same build can decrypt what it
             * encrypted. This is deterrence, not unbreakable DRM - see
             * `docs/content-encryption-plan.md`'s own "Motivation"
             * section for why that distinction matters.
             * @luaoutro
             * ## Key configuration
             *
             * Set at build time, never at runtime:
             *
             * ```shell
             * cmake -B build -S . -DSCARAB_CONTENT_KEY=<64 hex characters>
             * ```
             *
             * A 64-character hex string decodes to the 32 raw bytes
             * `crypto_secretbox`'s key requires. Left empty by default
             * — both primitives above fail cleanly (return `nil`, log a
             * clear error) rather than silently falling back to some
             * fixed, publicly-known placeholder key. Never commit a
             * real key to version control; pass it from a CI secret or
             * a local, gitignored file instead.
             */
            class LuaCryptoApi  {

                static int EncryptData( lua_State *pLuaState );
                static int DecryptData( lua_State *pLuaState );

                public:

                static void Register( lua_State *pLuaState );
            };
        }
    }
}

#endif  /* __LUACRYPTOAPI_H__ */
