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

#include <vector>

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

                /**
                 * @brief Attempt to decrypt a buffer that may have been
                 * produced by crypto_encrypt_data/tools/pack.lua, using
                 * this build's own SCARAB_CONTENT_KEY. NOT Lua-bound -
                 * this is the transparent runtime decrypt hook shared by
                 * LuaEngine::RunFile, LuaJsonApi::LoadJson, and
                 * LuaFileSystemApi::DoFile (see docs/content-encryption-
                 * plan.md, checkpoint 3), each calling this directly on
                 * whatever raw bytes SunLight::FileSystem::ReadFile()
                 * returns, before treating them as Lua source/JSON text -
                 * so a packed (encrypted) bundle and a loose,
                 * unencrypted one both just work, with no caller needing
                 * to know which it got ahead of time.
                 *
                 * @param data The raw bytes read from disk - may be
                 * plaintext (an unencrypted game, or this build has no
                 * key configured at all) or a genuine
                 * crypto_encrypt_data-produced blob.
                 * @param out Receives the decrypted bytes on success;
                 * left untouched on failure.
                 * @return true if data was successfully decrypted (this
                 * build has a valid key AND data authenticates against
                 * it) - false otherwise. False is the normal, expected
                 * outcome for plaintext content, not an error condition;
                 * callers should silently fall back to treating data as
                 * plaintext rather than logging anything (unlike
                 * crypto_decrypt_data itself, which DOES log - a Lua
                 * caller invoking it directly is always expecting
                 * encrypted input). A blob encrypted for a DIFFERENT key
                 * also returns false here (indistinguishable from
                 * plaintext, by design - see AEAD's own guarantee) - the
                 * caller then treats the still-encrypted bytes as
                 * plaintext, surfacing as an ordinary Lua/JSON parse
                 * error rather than a dedicated "wrong key" message.
                 */
                static bool TryDecryptBytes( const std :: vector<unsigned char> &data, std :: vector<unsigned char> &out );
            };
        }
    }
}

#endif  /* __LUACRYPTOAPI_H__ */
