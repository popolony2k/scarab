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

#include "lua/luapackapi.h"
#include "miniz.h"
#include <cstdio>
#include <cstring>
#include <fstream>
#include <filesystem>
#include <map>
#include <vector>


namespace fs = std :: filesystem;


namespace Scarab  {
    namespace Engine  {
        namespace Lua  {

            // Handle 0 is reserved for "invalid" (the same convention
            // sprite_acquire already uses) - real handles start at 1.
            // A plain incrementing counter + map is enough here: this
            // is single-threaded, dev-time tooling (tools/pack.lua),
            // not a hot runtime path, unlike SpritePool's own handle
            // scheme (packed generation+index, built for per-frame
            // reuse) which would be overkill for this.
            static std :: map<int, mz_zip_archive *>  s_ArchiveHandles;
            static int                                s_nNextHandle = 1;

            /**
             * @luaname{pack_list_directory(path) -> paths}
             * @luagroup{pack_fs}
             * @luaheading{Filesystem (native paths)}
             * @luadoc
             * Recursively lists every regular file under `path` (a
             * real native OS directory), returning a `1`-based,
             * `ipairs`-friendly table of paths *relative* to `path`
             * itself, each using `/` regardless of host OS (matching
             * this engine's own virtual-path convention elsewhere, so
             * an entry name built from one of these reads naturally
             * once it's inside a `.zip`). Returns an empty table (not
             * `nil`) if `path` doesn't exist or contains nothing.
             * @luaexample
             * for _, relativePath in ipairs(pack_list_directory(sourceDir)) do
             *   local data = pack_read_file(sourceDir .. "/" .. relativePath)
             *   -- ... encrypt data, pack_add_entry(archive, relativePath, encrypted) ...
             * end
             */
            int LuaPackApi :: ListDirectory( lua_State *pLuaState )  {

                const char  *szPath = lua_tostring( pLuaState, 1 );

                lua_newtable( pLuaState );

                std :: error_code  errorCode;
                fs :: path         basePath( szPath );

                if( !fs :: is_directory( basePath, errorCode ) )
                    return 1;

                int  nIndex = 1;

                for( const fs :: directory_entry &entry :
                        fs :: recursive_directory_iterator( basePath, fs :: directory_options :: skip_permission_denied, errorCode ) )  {

                    if( !entry.is_regular_file( errorCode ) )
                        continue;

                    fs :: path   relativePath = fs :: relative( entry.path(), basePath, errorCode );
                    std :: string  strRelative = relativePath.generic_string();  // generic_string() always uses '/'

                    lua_pushlstring( pLuaState, strRelative.c_str(), strRelative.size() );
                    lua_rawseti( pLuaState, -2, nIndex++ );
                }

                return 1;
            }

            /**
             * @luaname{pack_read_file(path) -> data}
             * @luagroup{pack_fs}
             * @luadoc
             * Reads the whole file at `path` (a real native OS path)
             * as raw bytes. Returns `nil` (and logs an error) if it
             * can't be opened.
             */
            int LuaPackApi :: ReadFile( lua_State *pLuaState )  {

                const char  *szPath = lua_tostring( pLuaState, 1 );

                std :: ifstream  file( szPath, std :: ios :: binary | std :: ios :: ate );

                if( !file.is_open() )  {
                    fprintf( stderr, "LuaPackApi: pack_read_file couldn't open [%s].\n", szPath );
                    lua_pushnil( pLuaState );

                    return 1;
                }

                std :: streamsize  nSize = file.tellg();

                file.seekg( 0, std :: ios :: beg );

                std :: vector<char>  data( ( size_t ) nSize );

                if( nSize > 0 && !file.read( data.data(), nSize ) )  {
                    fprintf( stderr, "LuaPackApi: pack_read_file failed reading [%s].\n", szPath );
                    lua_pushnil( pLuaState );

                    return 1;
                }

                lua_pushlstring( pLuaState, data.data(), data.size() );

                return 1;
            }

            /**
             * @luaname{pack_write_file(path, data) -> success}
             * @luagroup{pack_fs}
             * @luadoc
             * Writes `data` (raw bytes) to `path` (a real native OS
             * path), creating any missing parent directories first.
             */
            int LuaPackApi :: WriteFile( lua_State *pLuaState )  {

                const char  *szPath = lua_tostring( pLuaState, 1 );
                size_t       nDataLen;
                const char   *szData = lua_tolstring( pLuaState, 2, &nDataLen );

                fs :: path         targetPath( szPath );
                std :: error_code  errorCode;

                if( targetPath.has_parent_path() )
                    fs :: create_directories( targetPath.parent_path(), errorCode );

                std :: ofstream  file( szPath, std :: ios :: binary | std :: ios :: trunc );

                if( !file.is_open() || !file.write( szData, nDataLen ) )  {
                    fprintf( stderr, "LuaPackApi: pack_write_file couldn't write [%s].\n", szPath );
                    lua_pushboolean( pLuaState, false );

                    return 1;
                }

                lua_pushboolean( pLuaState, true );

                return 1;
            }

            /**
             * @luaname{pack_create_archive(path) -> handle}
             * @luagroup{pack_archive}
             * @luaheading{Archive (miniz-backed .zip writing)}
             * @luadoc
             * Creates a new `.zip` archive at `path` (a real native OS
             * path, overwritten if it already exists), ready for
             * `pack_add_entry` calls. Returns `0` on failure (path not
             * writable, etc).
             */
            int LuaPackApi :: CreateArchive( lua_State *pLuaState )  {

                const char  *szPath = lua_tostring( pLuaState, 1 );

                mz_zip_archive  *pZip = new mz_zip_archive();  // value-init zeroes it, same as memset - required by miniz's own contract

                if( !mz_zip_writer_init_file( pZip, szPath, 0 ) )  {
                    fprintf( stderr, "LuaPackApi: pack_create_archive couldn't create [%s].\n", szPath );
                    delete pZip;
                    lua_pushinteger( pLuaState, 0 );

                    return 1;
                }

                int  nHandle = s_nNextHandle++;

                s_ArchiveHandles[nHandle] = pZip;

                lua_pushinteger( pLuaState, nHandle );

                return 1;
            }

            /**
             * @luaname{pack_add_entry(handle, entryName, data) -> success}
             * @luagroup{pack_archive}
             * @luadoc
             * Adds `data` (raw bytes) to the archive `handle` (from
             * `pack_create_archive`) as a new entry named `entryName` -
             * typically one of `pack_list_directory`'s own returned
             * relative paths, after encrypting it's bytes via
             * `crypto_encrypt_data` (see LuaCryptoApi). Compressed at
             * miniz's own default level.
             */
            int LuaPackApi :: AddEntry( lua_State *pLuaState )  {

                int          nHandle    = ( int ) lua_tointeger( pLuaState, 1 );
                const char   *szEntry   = lua_tostring( pLuaState, 2 );
                size_t       nDataLen;
                const char   *szData    = lua_tolstring( pLuaState, 3, &nDataLen );

                auto  it = s_ArchiveHandles.find( nHandle );

                if( it == s_ArchiveHandles.end() )  {
                    fprintf( stderr, "LuaPackApi: pack_add_entry given an invalid archive handle [%d].\n", nHandle );
                    lua_pushboolean( pLuaState, false );

                    return 1;
                }

                bool  bResult = mz_zip_writer_add_mem( it->second, szEntry, szData, nDataLen, MZ_DEFAULT_COMPRESSION );

                if( !bResult )
                    fprintf( stderr, "LuaPackApi: pack_add_entry failed adding [%s].\n", szEntry );

                lua_pushboolean( pLuaState, bResult );

                return 1;
            }

            /**
             * @luaname{pack_close_archive(handle) -> success}
             * @luagroup{pack_archive}
             * @luadoc
             * Finalizes and closes the archive `handle` - the `.zip`
             * file on disk isn't actually valid/complete until this is
             * called. The handle itself is no longer usable after this
             * call either way (success or failure).
             */
            int LuaPackApi :: CloseArchive( lua_State *pLuaState )  {

                int  nHandle = ( int ) lua_tointeger( pLuaState, 1 );

                auto  it = s_ArchiveHandles.find( nHandle );

                if( it == s_ArchiveHandles.end() )  {
                    fprintf( stderr, "LuaPackApi: pack_close_archive given an invalid archive handle [%d].\n", nHandle );
                    lua_pushboolean( pLuaState, false );

                    return 1;
                }

                mz_zip_archive  *pZip = it->second;
                bool             bResult = mz_zip_writer_finalize_archive( pZip );

                mz_zip_writer_end( pZip );
                delete pZip;
                s_ArchiveHandles.erase( it );

                if( !bResult )
                    fprintf( stderr, "LuaPackApi: pack_close_archive failed to finalize handle [%d].\n", nHandle );

                lua_pushboolean( pLuaState, bResult );

                return 1;
            }

            /**
             * @brief Register the packaging-tool Lua-callable functions.
             */
            void LuaPackApi :: Register( lua_State *pLuaState )  {

                lua_register( pLuaState, "pack_list_directory", LuaPackApi :: ListDirectory );
                lua_register( pLuaState, "pack_read_file", LuaPackApi :: ReadFile );
                lua_register( pLuaState, "pack_write_file", LuaPackApi :: WriteFile );
                lua_register( pLuaState, "pack_create_archive", LuaPackApi :: CreateArchive );
                lua_register( pLuaState, "pack_add_entry", LuaPackApi :: AddEntry );
                lua_register( pLuaState, "pack_close_archive", LuaPackApi :: CloseArchive );
            }
        }
    }
}
