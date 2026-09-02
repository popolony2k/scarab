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

#include "lua/luacryptoapi.h"
#include <sodium.h>
#include <cstdio>
#include <cstring>
#include <vector>


namespace Scarab  {
    namespace Engine  {
        namespace Lua  {

            /**
             * @brief Lazily decodes the compile-time SCARAB_CONTENT_KEY
             * (a "${SCARAB_CONTENT_KEY}" CMake string, hex-encoded) into
             * it's raw crypto_secretbox_KEYBYTES (32) bytes, once, and
             * caches the result for every later call. A missing/empty or
             * malformed key (anything other than exactly 64 valid hex
             * characters) is a normal, expected configuration state -
             * not every build needs content encryption - so this
             * doesn't abort or throw, it just leaves the key unusable
             * and lets EncryptData/DecryptData report that cleanly to
             * their own Lua caller instead.
             *
             * @return A pointer to the decoded 32-byte key, or nullptr
             * if SCARAB_CONTENT_KEY isn't a valid 64-character hex
             * string (including the common case of it being empty,
             * i.e. never set at build time at all).
             */
            static const unsigned char* GetContentKey( void )  {

                static unsigned char  s_aKey[crypto_secretbox_KEYBYTES];
                static bool           s_bResolved = false;
                static bool           s_bValid     = false;

                if( s_bResolved )
                    return s_bValid ? s_aKey : nullptr;

                s_bResolved = true;

                static const char  *szHexKey = SCARAB_CONTENT_KEY;
                size_t              nHexLen  = std :: strlen( szHexKey );

                if( nHexLen != ( crypto_secretbox_KEYBYTES * 2 ) )  {
                    if( nHexLen != 0 )  {
                        fprintf( stderr, "LuaCryptoApi: SCARAB_CONTENT_KEY must be exactly %d hex "
                            "characters (got %zu) - content encryption is disabled for this build.\n",
                            crypto_secretbox_KEYBYTES * 2, nHexLen );
                    }

                    return nullptr;
                }

                size_t  nDecodedLen = 0;

                if( ( sodium_hex2bin( s_aKey, sizeof( s_aKey ), szHexKey, nHexLen,
                        nullptr, &nDecodedLen, nullptr ) != 0 )
                    || ( nDecodedLen != crypto_secretbox_KEYBYTES ) )  {
                    fprintf( stderr, "LuaCryptoApi: SCARAB_CONTENT_KEY is not valid hex - "
                        "content encryption is disabled for this build.\n" );

                    return nullptr;
                }

                s_bValid = true;

                return s_aKey;
            }

            /**
             * @luaname{crypto_encrypt_data(data) -> encrypted}
             * @luagroup{crypto_data}
             * @luadoc
             * Encrypts `data` (any Lua string - binary-safe, may
             * contain embedded zero bytes) using the key compiled into
             * this build (`SCARAB_CONTENT_KEY` - see this class's own
             * doc above). Returns the encrypted blob as a Lua string,
             * ready to write straight to a file, or `nil` (and logs an
             * error) if this build has no valid key configured. A
             * fresh random nonce is generated for every call and
             * prepended to the returned blob - callers never handle
             * the nonce themselves, `crypto_decrypt_data` reads it back
             * out automatically.
             * @luaexample
             * local encrypted = crypto_encrypt_data(plainBytes)
             *
             * if encrypted == nil then
             *   error("content encryption not configured for this build")
             * end
             */
            int LuaCryptoApi :: EncryptData( lua_State *pLuaState )  {

                const unsigned char  *pKey = GetContentKey();

                if( pKey == nullptr )  {
                    lua_pushnil( pLuaState );

                    return 1;
                }

                size_t       nPlainLen;
                const char   *szPlain = lua_tolstring( pLuaState, 1, &nPlainLen );

                std :: vector<unsigned char>  blob( crypto_secretbox_NONCEBYTES + crypto_secretbox_MACBYTES + nPlainLen );
                unsigned char  *pNonce      = blob.data();
                unsigned char  *pCiphertext = blob.data() + crypto_secretbox_NONCEBYTES;

                randombytes_buf( pNonce, crypto_secretbox_NONCEBYTES );

                crypto_secretbox_easy( pCiphertext, ( const unsigned char * ) szPlain, nPlainLen, pNonce, pKey );

                lua_pushlstring( pLuaState, ( const char * ) blob.data(), blob.size() );

                return 1;
            }

            /**
             * @brief Shared core behind both crypto_decrypt_data
             * (DecryptData below) and the transparent runtime hook
             * (TryDecryptBytes, this class's own public interface) - one
             * real implementation of "split a blob into nonce/ciphertext
             * and call crypto_secretbox_open_easy", rather than two
             * copies of buffer-arithmetic-adjacent-to-security-code that
             * could silently drift apart. Deliberately logs nothing on
             * failure either way - DecryptData/TryDecryptBytes each
             * decide for themselves whether a failure here is worth
             * reporting (see TryDecryptBytes's own doc comment for why
             * it stays silent).
             *
             * @return true and fills out on success; false (out left
             * untouched) if pKey is null, blob is too short to be
             * valid, or the AEAD authentication check itself fails.
             */
            static bool DecryptCore( const unsigned char *pKey, const unsigned char *pBlob, size_t nBlobLen,
                    std :: vector<unsigned char> &out )  {

                if( ( pKey == nullptr ) || ( nBlobLen < ( crypto_secretbox_NONCEBYTES + crypto_secretbox_MACBYTES ) ) )
                    return false;

                const unsigned char  *pNonce      = pBlob;
                const unsigned char  *pCiphertext = pBlob + crypto_secretbox_NONCEBYTES;
                size_t                nCipherLen  = nBlobLen - crypto_secretbox_NONCEBYTES;

                std :: vector<unsigned char>  plain( nCipherLen - crypto_secretbox_MACBYTES );

                if( crypto_secretbox_open_easy( plain.data(), pCiphertext, nCipherLen, pNonce, pKey ) != 0 )
                    return false;

                out = std :: move( plain );

                return true;
            }

            /**
             * @luaname{crypto_decrypt_data(encrypted) -> data}
             * @luagroup{crypto_data}
             * @luadoc
             * The inverse of `crypto_encrypt_data` - decrypts a blob it
             * produced, using this build's own compiled-in key. Returns
             * the original data as a Lua string, or `nil` (and logs an
             * error) either if this build has no valid key configured,
             * or if decryption itself fails - a wrong key, truncated
             * data, or genuine tampering all report the same way, by
             * design (`crypto_secretbox_open_easy` is authenticated -
             * it detects any modification, it doesn't just silently
             * return garbage).
             * @luaexample
             * local data = crypto_decrypt_data(encrypted)
             *
             * if data == nil then
             *   error("failed to decrypt - wrong key or corrupted data")
             * end
             */
            int LuaCryptoApi :: DecryptData( lua_State *pLuaState )  {

                const unsigned char  *pKey = GetContentKey();

                if( pKey == nullptr )  {
                    lua_pushnil( pLuaState );

                    return 1;
                }

                size_t       nBlobLen;
                const char   *szBlob = lua_tolstring( pLuaState, 1, &nBlobLen );

                std :: vector<unsigned char>  plain;

                if( !DecryptCore( pKey, ( const unsigned char * ) szBlob, nBlobLen, plain ) )  {
                    fprintf( stderr, "LuaCryptoApi: crypto_decrypt_data failed - wrong key, corrupted data, "
                        "or a blob too short to be valid.\n" );
                    lua_pushnil( pLuaState );

                    return 1;
                }

                lua_pushlstring( pLuaState, ( const char * ) plain.data(), plain.size() );

                return 1;
            }

            /**
             * @brief See this method's own declaration (luacryptoapi.h)
             * for the full rationale - the transparent runtime decrypt
             * hook shared by LuaEngine::RunFile/LuaJsonApi::LoadJson/
             * LuaFileSystemApi::DoFile.
             */
            bool LuaCryptoApi :: TryDecryptBytes( const std :: vector<unsigned char> &data, std :: vector<unsigned char> &out )  {

                return DecryptCore( GetContentKey(), data.data(), data.size(), out );
            }

            /**
             * @brief Register the content-encryption Lua-callable
             * functions. sodium_init() is called once here rather than
             * lazily inside EncryptData/DecryptData - Register() itself
             * already only ever runs once, at engine startup, matching
             * every other Lua*Api's own one-time setup (e.g.
             * RaylibEngine's constructor registering it's own raylib
             * callback once). libsodium's own docs describe
             * sodium_init() as safe to call more than once regardless,
             * so this isn't strictly required for correctness - just
             * the more explicit, "set up once at startup" shape this
             * codebase already follows elsewhere.
             */
            void LuaCryptoApi :: Register( lua_State *pLuaState )  {

                if( sodium_init() < 0 )  {
                    fprintf( stderr, "LuaCryptoApi: sodium_init() failed - content encryption unavailable.\n" );
                }

                lua_register( pLuaState, "crypto_encrypt_data", LuaCryptoApi :: EncryptData );
                lua_register( pLuaState, "crypto_decrypt_data", LuaCryptoApi :: DecryptData );
            }
        }
    }
}
