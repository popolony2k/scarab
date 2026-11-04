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
#include "lua/luaengineutil.h"
#include "filesystem/filesystemfactory.h"
#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdint>
#include <unordered_set>

extern "C"  {
  #include "lauxlib.h"
}

#define __UTF8_CONTINUATION         0x80   // 10xxxxxx: a continuation byte (and the first non-ASCII byte value)
#define __UTF8_CONTINUATION_MASK    0xC0   // the bits that tell a continuation byte apart
#define __UTF8_CONTINUATION_BITS    0x3F   // a continuation byte's payload
#define __UTF8_CONTINUATION_SHIFT   6      // payload bits per continuation byte
#define __UTF8_TWO_BYTE_LEAD        0xC0   // 110xxxxx
#define __UTF8_THREE_BYTE_LEAD      0xE0   // 1110xxxx
#define __UTF8_FOUR_BYTE_LEAD       0xF0   // 11110xxx
#define __UTF8_INVALID_LEAD         0xF8   // 11111xxx never starts a sequence
#define __UTF8_TWO_BYTE_EXTRA       1      // continuation bytes after each kind of lead
#define __UTF8_THREE_BYTE_EXTRA     2
#define __UTF8_FOUR_BYTE_EXTRA      3
#define __UTF8_TWO_BYTE_BITS        0x1F   // a two-byte lead's payload
#define __UTF8_THREE_BYTE_BITS      0x0F   // a three-byte lead's payload
#define __UTF8_FOUR_BYTE_BITS       0x07   // a four-byte lead's payload
#define __UTF8_TWO_BYTE_MINIMUM     0x80   // below it, a two-byte form is overlong
#define __UTF8_THREE_BYTE_MINIMUM   0x800
#define __UTF8_FOUR_BYTE_MINIMUM    0x10000
#define __UNICODE_MAXIMUM           0x10FFFF
#define __UTF16_SURROGATE_FIRST     0xD800  // surrogates are not characters, so never in UTF-8
#define __UTF16_SURROGATE_LAST      0xDFFF


namespace Scarab  {
    namespace Engine  {
        namespace Lua  {

            /**
             * @brief Push a JSON value onto the Lua stack as its equivalent Lua value (a table for
             * objects and arrays, a scalar otherwise), one level of a depth-limited walk.
             *
             * @param pLuaState Lua state to be used by engine call;
             * @param value The JSON value to convert and push;
             * @param nDepth How many tables deep this value is (1 for the document itself);
             * @param strError Receives the reason on failure;
             * @return true if the value was pushed; false (nothing pushed) when it nests deeper than
             * MAX_DEPTH or the Lua stack can't grow;
             */
            static bool PushJsonValueAt( lua_State* pLuaState, const nlohmann :: json& value, int nDepth, std :: string& strError )  {

                if( value.is_structured() && ( nDepth > LuaJsonApi :: MAX_DEPTH ) )  {
                    strError = "nested more than " + std :: to_string( LuaJsonApi :: MAX_DEPTH ) + " tables deep";
                    return false;
                }

                if( !lua_checkstack( pLuaState, 2 ) )  {
                    strError = "out of Lua stack space";
                    return false;
                }

                if( value.is_object() )  {
                    lua_newtable( pLuaState );

                    for( auto it = value.begin(); it != value.end(); ++it )  {
                        if( !PushJsonValueAt( pLuaState, it.value(), nDepth + 1, strError ) )  {
                            lua_pop( pLuaState, 1 );
                            return false;
                        }

                        lua_setfield( pLuaState, -2, it.key().c_str() );
                    }
                }
                else if( value.is_array() )  {
                    lua_newtable( pLuaState );

                    lua_Integer  nIndex = 1;

                    for( const nlohmann :: json& item : value )  {
                        if( !PushJsonValueAt( pLuaState, item, nDepth + 1, strError ) )  {
                            lua_pop( pLuaState, 1 );
                            return false;
                        }

                        lua_rawseti( pLuaState, -2, nIndex++ );
                    }
                }
                else if( value.is_boolean() )
                    lua_pushboolean( pLuaState, value.get<bool>() );
                else if( value.is_number_integer() )
                    lua_pushinteger( pLuaState, value.get<lua_Integer>() );
                else if( value.is_number() )
                    lua_pushnumber( pLuaState, value.get<double>() );
                else if( value.is_string() )  {
                    const std :: string&  strValue = value.get_ref<const std :: string&>();

                    lua_pushlstring( pLuaState, strValue.data(), strValue.size() );
                }
                else
                    lua_pushnil( pLuaState );

                return true;
            }

            /**
             * @brief Push a JSON value onto the Lua stack as its equivalent Lua value (a table for
             * objects and arrays, a scalar otherwise; JSON null is nil).
             *
             * @param pLuaState Lua state to be used by engine call;
             * @param value The JSON value to convert and push;
             * @param strError Receives the reason on failure;
             * @return true if the value was pushed; false (nothing pushed) when it nests more than
             * MAX_DEPTH tables deep - a deeper file would overflow Lua's stack, not just be slow;
             */
            bool LuaJsonApi :: PushJsonValue( lua_State* pLuaState, const nlohmann :: json& value, std :: string& strError )  {

                return PushJsonValueAt( pLuaState, value, 1, strError );
            }

            /**
             * @brief Whether a text is valid UTF-8: no stray continuation byte, no overlong form, no
             * surrogate (U+D800..U+DFFF) and nothing above U+10FFFF - what JSON text must be.
             *
             * @param strText The bytes to check;
             * @return true if every byte belongs to a valid UTF-8 sequence;
             */
            bool LuaJsonApi :: IsValidUtf8( const std :: string& strText )  {

                const unsigned char*  pText   = reinterpret_cast<const unsigned char *>( strText.data() );
                size_t                nLength = strText.size();
                size_t                nPos    = 0;

                while( nPos < nLength )  {
                    unsigned char  nLead = pText[nPos];
                    size_t         nExtra;
                    unsigned int   nCodePoint;
                    unsigned int   nMinimum;

                    if( nLead < __UTF8_TWO_BYTE_LEAD )  {
                        if( nLead >= __UTF8_CONTINUATION )        // a continuation byte with no lead
                            return false;

                        nPos++;
                        continue;
                    }
                    else if( nLead < __UTF8_THREE_BYTE_LEAD )  {
                        nExtra     = __UTF8_TWO_BYTE_EXTRA;
                        nCodePoint = nLead & __UTF8_TWO_BYTE_BITS;
                        nMinimum   = __UTF8_TWO_BYTE_MINIMUM;
                    }
                    else if( nLead < __UTF8_FOUR_BYTE_LEAD )  {
                        nExtra     = __UTF8_THREE_BYTE_EXTRA;
                        nCodePoint = nLead & __UTF8_THREE_BYTE_BITS;
                        nMinimum   = __UTF8_THREE_BYTE_MINIMUM;
                    }
                    else if( nLead < __UTF8_INVALID_LEAD )  {
                        nExtra     = __UTF8_FOUR_BYTE_EXTRA;
                        nCodePoint = nLead & __UTF8_FOUR_BYTE_BITS;
                        nMinimum   = __UTF8_FOUR_BYTE_MINIMUM;
                    }
                    else
                        return false;

                    if( nPos + nExtra >= nLength )            // the sequence runs past the end
                        return false;

                    for( size_t nByte = 1; nByte <= nExtra; nByte++ )  {
                        unsigned char  nNext = pText[nPos + nByte];

                        if( ( nNext & __UTF8_CONTINUATION_MASK ) != __UTF8_CONTINUATION )
                            return false;

                        nCodePoint = ( nCodePoint << __UTF8_CONTINUATION_SHIFT ) | ( nNext & __UTF8_CONTINUATION_BITS );
                    }

                    if( ( nCodePoint < nMinimum ) || ( nCodePoint > __UNICODE_MAXIMUM ) ||
                        ( ( nCodePoint >= __UTF16_SURROGATE_FIRST ) && ( nCodePoint <= __UTF16_SURROGATE_LAST ) ) )
                        return false;

                    nPos += nExtra + 1;
                }

                return true;
            }

            /**
             * @brief Whether a table key can be written after a '.' in a path shown to the user (a Lua
             * identifier); any other key is shown as ["key"].
             *
             * @param strKey The key;
             * @return true if it is a Lua identifier;
             */
            static bool IsIdentifier( const std :: string& strKey )  {

                if( strKey.empty() || std :: isdigit( static_cast<unsigned char>( strKey[0] ) ) )
                    return false;

                for( char cChar : strKey )  {
                    if( !std :: isalnum( static_cast<unsigned char>( cChar ) ) && ( cChar != '_' ) )
                        return false;
                }

                return true;
            }

            /**
             * @brief The path of a table's field, as a refusal shows it: "value.name" or "value[\"a b\"]".
             *
             * @param strParent The table's own path;
             * @param strKey The field's key;
             * @return The field's path;
             */
            static std :: string FieldPath( const std :: string& strParent, const std :: string& strKey )  {

                return IsIdentifier( strKey ) ? ( strParent + "." + strKey ) : ( strParent + "[\"" + strKey + "\"]" );
            }

            static bool ValueToJson( lua_State* pLuaState, int nIndex, const std :: string& strPath, int nDepth,
                                     std :: unordered_set<const void *>& branch, nlohmann :: json& out, std :: string& strError );

            /**
             * @brief Convert a Lua table to JSON: an array when its keys are exactly 1..n, an object when
             * they are all strings, {} when it is empty; anything else is refused, naming where.
             *
             * @param pLuaState Lua state to be used by engine call;
             * @param nIndex The table's stack index;
             * @param strPath The table's path, for a refusal (eg. "value.scores");
             * @param nDepth How many tables deep it is (1 for the value itself);
             * @param branch The tables being converted on the way down to this one (a cycle check);
             * @param out Receives the JSON;
             * @param strError Receives the reason on failure;
             * @return true if the table could be converted;
             */
            static bool TableToJson( lua_State* pLuaState, int nIndex, const std :: string& strPath, int nDepth,
                                     std :: unordered_set<const void *>& branch, nlohmann :: json& out, std :: string& strError )  {

                const void*  pTable = lua_topointer( pLuaState, nIndex );

                if( nDepth > LuaJsonApi :: MAX_DEPTH )  {
                    strError = strPath + " is nested more than " + std :: to_string( LuaJsonApi :: MAX_DEPTH ) + " tables deep";
                    return false;
                }

                if( branch.count( pTable ) > 0 )  {
                    strError = strPath + " contains itself";
                    return false;
                }

                if( !lua_checkstack( pLuaState, 3 ) )  {
                    strError = strPath + ": out of Lua stack space";
                    return false;
                }

                lua_Integer  nIntegerKeys = 0;
                lua_Integer  nLowestKey   = 0;
                lua_Integer  nHighestKey  = 0;
                lua_Integer  nStringKeys  = 0;

                lua_pushnil( pLuaState );

                while( lua_next( pLuaState, nIndex ) != 0 )  {
                    if( lua_isinteger( pLuaState, -2 ) )  {
                        lua_Integer  nKey = lua_tointeger( pLuaState, -2 );

                        nLowestKey  = ( nIntegerKeys == 0 ) ? nKey : std :: min( nLowestKey, nKey );
                        nHighestKey = ( nIntegerKeys == 0 ) ? nKey : std :: max( nHighestKey, nKey );
                        nIntegerKeys++;
                    }
                    else if( lua_type( pLuaState, -2 ) == LUA_TSTRING )
                        nStringKeys++;
                    else  {
                        int  nKeyType = lua_type( pLuaState, -2 );

                        strError = strPath + " has a " + ( ( nKeyType == LUA_TNUMBER ) ? std :: string( "non-integer number" )
                                                                                      : std :: string( lua_typename( pLuaState, nKeyType ) ) ) +
                                   " key (only string keys, or the integer keys 1..n of an array)";
                        lua_pop( pLuaState, 2 );
                        return false;
                    }

                    lua_pop( pLuaState, 1 );
                }

                if( ( nIntegerKeys > 0 ) && ( nStringKeys > 0 ) )  {
                    strError = strPath + " mixes integer and string keys";
                    return false;
                }

                if( ( nIntegerKeys > 0 ) && ( ( nLowestKey != 1 ) || ( nHighestKey != nIntegerKeys ) ) )  {
                    strError = strPath + " has integer keys that are not 1..n (a hole, or a key below 1)";
                    return false;
                }

                branch.insert( pTable );

                if( nIntegerKeys > 0 )  {
                    out = nlohmann :: json :: array();

                    for( lua_Integer nKey = 1; nKey <= nIntegerKeys; nKey++ )  {
                        nlohmann :: json  item;

                        lua_rawgeti( pLuaState, nIndex, nKey );

                        bool  bConverted = ValueToJson( pLuaState, lua_gettop( pLuaState ), strPath + "[" + std :: to_string( nKey ) + "]",
                                                        nDepth, branch, item, strError );

                        lua_pop( pLuaState, 1 );

                        if( !bConverted )
                            return false;

                        out.push_back( std :: move( item ) );
                    }
                }
                else  {
                    out = nlohmann :: json :: object();

                    lua_pushnil( pLuaState );

                    while( lua_next( pLuaState, nIndex ) != 0 )  {
                        size_t             nKeyLength = 0;
                        const char*        szKey      = lua_tolstring( pLuaState, -2, &nKeyLength );
                        std :: string      strKey( szKey, nKeyLength );
                        nlohmann :: json   item;

                        if( !LuaJsonApi :: IsValidUtf8( strKey ) )  {
                            strError = strPath + " has a key that is not valid UTF-8";
                            lua_pop( pLuaState, 2 );
                            return false;
                        }

                        if( !ValueToJson( pLuaState, lua_gettop( pLuaState ), FieldPath( strPath, strKey ), nDepth, branch, item, strError ) )  {
                            lua_pop( pLuaState, 2 );
                            return false;
                        }

                        out[strKey] = std :: move( item );
                        lua_pop( pLuaState, 1 );
                    }
                }

                branch.erase( pTable );

                return true;
            }

            /**
             * @brief Convert one Lua value to JSON: a boolean, an integer (kept an integer), a finite
             * float, a valid UTF-8 string or a table; anything else is refused, naming where.
             *
             * @param pLuaState Lua state to be used by engine call;
             * @param nIndex The value's stack index;
             * @param strPath The value's path, for a refusal;
             * @param nDepth How many tables deep the table holding it is (0 for the value itself);
             * @param branch The tables being converted on the way down to this value (a cycle check);
             * @param out Receives the JSON;
             * @param strError Receives the reason on failure;
             * @return true if the value could be converted;
             */
            static bool ValueToJson( lua_State* pLuaState, int nIndex, const std :: string& strPath, int nDepth,
                                     std :: unordered_set<const void *>& branch, nlohmann :: json& out, std :: string& strError )  {

                switch( lua_type( pLuaState, nIndex ) )  {
                    case LUA_TBOOLEAN :
                        out = ( lua_toboolean( pLuaState, nIndex ) != 0 );
                        return true;

                    case LUA_TNUMBER :
                        if( lua_isinteger( pLuaState, nIndex ) )  {
                            out = static_cast<std :: int64_t>( lua_tointeger( pLuaState, nIndex ) );
                            return true;
                        }

                        if( !std :: isfinite( lua_tonumber( pLuaState, nIndex ) ) )  {
                            strError = strPath + " is NaN or infinity";
                            return false;
                        }

                        out = static_cast<double>( lua_tonumber( pLuaState, nIndex ) );
                        return true;

                    case LUA_TSTRING :  {
                        size_t         nLength = 0;
                        const char*    szValue = lua_tolstring( pLuaState, nIndex, &nLength );
                        std :: string  strValue( szValue, nLength );

                        if( !LuaJsonApi :: IsValidUtf8( strValue ) )  {
                            strError = strPath + " is a string that is not valid UTF-8";
                            return false;
                        }

                        out = std :: move( strValue );
                        return true;
                    }

                    case LUA_TTABLE :
                        return TableToJson( pLuaState, nIndex, strPath, nDepth + 1, branch, out, strError );

                    default :
                        strError = strPath + " is a " + lua_typename( pLuaState, lua_type( pLuaState, nIndex ) );
                        return false;
                }
            }

            /**
             * @brief Convert a Lua value to JSON, strictly, so that reading the JSON back gives the same
             * value (plans/MASTER_PLAN.md item C1c). A table whose keys are exactly 1..n becomes an
             * array, one with only string keys an object, an empty one {}; integers stay integers and
             * floats floats. Refused, with where: mixed keys, holes, any other kind of key, a function,
             * userdata or thread, a table that contains itself, nesting deeper than MAX_DEPTH, NaN and
             * infinity, and text that is not valid UTF-8. A table met twice on different branches is
             * fine (it is written twice).
             *
             * @param pLuaState Lua state to be used by engine call;
             * @param nIndex The value's stack index (the stack is left as it was);
             * @param strRootName What a refusal calls the value itself (eg. "value" -> "value.scores[3]");
             * @param out Receives the JSON;
             * @param strError Receives the reason on failure, starting with the path (eg. "value.f is a
             * function");
             * @return true if the value could be converted;
             */
            bool LuaJsonApi :: ToJson( lua_State* pLuaState, int nIndex, const std :: string& strRootName,
                                       nlohmann :: json& out, std :: string& strError )  {

                std :: unordered_set<const void *>  branch;

                return ValueToJson( pLuaState, lua_absindex( pLuaState, nIndex ), strRootName, 0, branch, out, strError );
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
             * @param pLuaState Lua state to be used by engine call;
             * @return int number of return data (if any - required by lua engine)
             *
             * @luaname{load_json(path) -> table | nil, message}
             * @luadoc
             * Read a JSON file and return its content as an equivalent Lua
             * table — this is the engine's only config-file bridge;
             * everything else under `resources/configs/` is read this
             * way rather than through any Caravellius-specific loader.
             * Returns `nil, "message"` (never a Lua error) if the file
             * can't be opened (`"cannot open '<path>'"`) or fails to parse
             * (`"cannot parse '<path>': <reason>"`).
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
             * A file nested more than 64 objects/arrays deep is refused
             * (`"cannot read '<path>': nested more than 64 tables deep"`)
             * rather than overflowing Lua's stack.
             *
             * Transparently reads content encrypted by
             * `crypto_encrypt_data`/`tools/pack.lua` too — nothing to
             * opt into on the calling side; this build's own
             * `SCARAB_CONTENT_KEY` is tried automatically, falling back
             * to plain JSON if it doesn't apply (no key configured, or
             * the file just isn't encrypted).
             */
            int LuaJsonApi :: LoadJson( lua_State* pLuaState )  {

                std :: string                 strError;
                std :: string                 strPath;
                std :: vector<unsigned char>  data;

                if( !LuaEngineUtil :: ReadStringArg( pLuaState, 1, "path", strPath, strError ) )
                    return LuaEngineUtil :: PushError( pLuaState, strError );

                if( !SunLight :: FileSystem :: FileSystemFactory :: GetFileSystem().ReadFile( strPath, data ) )
                    return LuaEngineUtil :: PushError( pLuaState, "cannot open '" + strPath + "'" );

                // Transparent content-encryption support (see the root
                // README.md's own "Content encryption" section) - see
                // LuaEngine::RunFile's own identical hook for the full
                // rationale; silently falls back to the raw bytes as-is
                // (plaintext) if decryption doesn't apply.
                std :: vector<unsigned char>  decrypted;

                if( LuaCryptoApi :: TryDecryptBytes( data, decrypted ) )
                    data = std :: move( decrypted );

                nlohmann :: json  jsonData;

                try  {
                    jsonData = nlohmann :: json :: parse( data.begin(), data.end() );
                }
                catch( const nlohmann :: json :: parse_error& ex )  {
                    return LuaEngineUtil :: PushError( pLuaState, "cannot parse '" + strPath + "': " + ex.what() );
                }

                if( !PushJsonValue( pLuaState, jsonData, strError ) )
                    return LuaEngineUtil :: PushError( pLuaState, "cannot read '" + strPath + "': " + strError );

                return 1;
            }

            /**
             * @brief Register the JSON bridge Lua-callable functions.
             *
             * @param pLuaState Lua state to be used by engine call;
             */
            void LuaJsonApi :: Register( lua_State* pLuaState )  {

                lua_register( pLuaState, "load_json", LuaJsonApi :: LoadJson );
            }
        }
    }
}
