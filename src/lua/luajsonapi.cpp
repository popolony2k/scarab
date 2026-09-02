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

#include "lua/luajsonapi.h"
#include "lua/luacryptoapi.h"
#include "filesystem/filesystemfactory.h"
#include <nlohmann/json.hpp>

extern "C"
{
  #include "lauxlib.h"
}


namespace Scarab  {
    namespace Engine  {
        namespace Lua  {

            /**
             * @brief Recursively push a JSON value onto the Lua stack as its
             * equivalent Lua value (a table for objects/arrays, a scalar otherwise).
             *
             * @param pLuaState Lua state to be used by engine call.
             * @param value The JSON value to convert and push;
             */
            static void PushJsonValue( lua_State *pLuaState, const nlohmann :: json &value )  {

                if( value.is_object() )  {
                    lua_newtable( pLuaState );

                    for( auto it = value.begin(); it != value.end(); ++it )  {
                        PushJsonValue( pLuaState, it.value() );
                        lua_setfield( pLuaState, -2, it.key().c_str() );
                    }
                }
                else if( value.is_array() )  {
                    lua_newtable( pLuaState );

                    int   nIndex = 1;

                    for( const nlohmann :: json &item : value )  {
                        PushJsonValue( pLuaState, item );
                        lua_rawseti( pLuaState, -2, nIndex++ );
                    }
                }
                else if( value.is_boolean() )
                    lua_pushboolean( pLuaState, value.get<bool>() );
                else if( value.is_number_integer() )
                    lua_pushinteger( pLuaState, value.get<lua_Integer>() );
                else if( value.is_number() )
                    lua_pushnumber( pLuaState, value.get<double>() );
                else if( value.is_string() )
                    lua_pushstring( pLuaState, value.get<std :: string>().c_str() );
                else
                    lua_pushnil( pLuaState );
            }

            /**
             * @brief Load a JSON file and return it's content as an equivalent Lua table.
             *
             * Reads the file via SunLight::FileSystem (Phase 12 prototype,
             * game-engine repo) rather than a raw std::ifstream, matching
             * every other subsystem already routed through
             * FileSystemFactory (textures/sound/tilemaps) - the same
             * mount (a real loose directory today, potentially a real
             * archive later) now backs every resource read this engine
             * does, config files included. nlohmann::json parses directly
             * from the in-memory byte range ReadFile() returns - no
             * std::ifstream/temporary buffer needed.
             *
             * @param pLuaState Lua state to be used by engine call.
             * @return int number of return data (if any - required by lua engine)
             *
             * @luaname{load_json(path) -> table}
             * @luadoc
             * Read a JSON file and return its content as an equivalent Lua
             * table — this is the engine's only config-file bridge;
             * everything else under `resources/configs/` is read this
             * way rather than through any Caravellius-specific loader.
             * Returns `nil` (and logs an error to stderr) if the file
             * can't be opened or fails to parse.
             *
             * JSON objects become Lua tables keyed by string; JSON arrays
             * become Lua tables with sequential integer keys (`1`-based,
             * matching Lua convention, via `ipairs`-friendly numbering —
             * not JSON's own `0`-based indexing).
             *
             * ```lua
             * local data = load_json(BASE_PATH .. "configs/soundfile.json")
             *
             * for _, pair in ipairs(data) do
             *   print(pair[1], pair[2].file_name)
             * end
             * ```
             *
             * ```json
             * [
             *   ["ID_CARAVELLIUS_SHOOT_AUDIO", { "file_name": "/audio/global/caravellius-shot.wav" }]
             * ]
             * ```
             *
             * Given the JSON above, `data[1][1]` is
             * `"ID_CARAVELLIUS_SHOOT_AUDIO"` and `data[1][2].file_name` is
             * `"/audio/global/caravellius-shot.wav"`.
             *
             * `load_json` is fully generic — it has no concept of any
             * particular file's schema, it just walks whatever structure
             * the JSON actually contains. Nested objects/arrays convert
             * recursively; numbers, strings, booleans, and `null` (→ Lua
             * `nil`) convert directly.
             *
             * Transparently reads content encrypted by
             * `crypto_encrypt_data`/`tools/pack.lua` too — nothing to
             * opt into on the calling side; this build's own
             * `SCARAB_CONTENT_KEY` is tried automatically, falling back
             * to plain JSON if it doesn't apply (no key configured, or
             * the file just isn't encrypted).
             */
            int LuaJsonApi :: LoadJson( lua_State *pLuaState )  {

                if( lua_gettop( pLuaState ) != 1 )  {
                    fprintf( stderr, "Invalid number of arguments on load_json call.\n" );
                    lua_pushnil( pLuaState );

                    return 1;
                }

                const char                     *szFileName = lua_tostring( pLuaState, 1 );
                std :: vector<unsigned char>  data;

                if( !SunLight :: FileSystem :: FileSystemFactory :: GetFileSystem().ReadFile( szFileName, data ) )  {
                    fprintf( stderr, "load_json: Unable to open file [%s].\n", szFileName );
                    lua_pushnil( pLuaState );

                    return 1;
                }

                // Transparent content-encryption support (see the root
                // README.md's own "Content encryption" section) - see
                // LuaEngine::RunFile's own identical hook for the full
                // rationale; silently falls back to the raw bytes as-is
                // (plaintext) if decryption doesn't apply.
                std :: vector<unsigned char>  decrypted;

                if( LuaCryptoApi :: TryDecryptBytes( data, decrypted ) )
                    data = std :: move( decrypted );

                try  {
                    nlohmann :: json   jsonData = nlohmann :: json :: parse( data.begin(), data.end() );

                    PushJsonValue( pLuaState, jsonData );
                }
                catch( const nlohmann :: json :: parse_error &ex )  {
                    fprintf( stderr, "load_json: Parse error on file [%s]: %s\n", szFileName, ex.what() );
                    lua_pushnil( pLuaState );
                }

                return 1;
            }

            /**
             * @brief Register the JSON bridge Lua-callable functions.
             *
             * @param pLuaState Lua state to be used by engine call.
             */
            void LuaJsonApi :: Register( lua_State *pLuaState )  {

                lua_register( pLuaState, "load_json", LuaJsonApi :: LoadJson );
            }
        }
    }
}
