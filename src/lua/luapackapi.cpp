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
#include "lua/luaengineutil.h"
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
             * @luaname{pack_list_directory(path) -> paths | nil, message}
             * @luagroup{pack_fs}
             * @luaheading{Filesystem (native paths)}
             * @luadoc
             * Recursively lists every regular file under `path` (a
             * real native OS directory), returning a `1`-based,
             * `ipairs`-friendly table of paths *relative* to `path`
             * itself, each using `/` regardless of host OS (matching
             * this engine's own virtual-path convention elsewhere, so
             * an entry name built from one of these reads naturally
             * once it's inside a `.zip`). Returns an empty table if
             * `path` contains nothing, and `nil, "message"` if it isn't
             * a directory. Every `pack_*` function reports a failure as
             * `nil, "message"` (never a Lua error, nothing printed);
             * one with nothing else to return returns `true`.
             * @luaexample
             * for _, relativePath in ipairs(pack_list_directory(sourceDir)) do
             *   local data = pack_read_file(sourceDir .. "/" .. relativePath)
             *   -- ... encrypt data, pack_add_entry(archive, relativePath, encrypted) ...
             * end
             */
            int LuaPackApi :: ListDirectory( lua_State* pLuaState )  {

                std :: string      strError;
                std :: string      strPath;
                std :: error_code  errorCode;

                if( !LuaEngineUtil :: ReadStringArg( pLuaState, 1, "path", strPath, strError ) )
                    return LuaEngineUtil :: PushError( pLuaState, strError );

                fs :: path  basePath( strPath );

                if( !fs :: is_directory( basePath, errorCode ) )
                    return LuaEngineUtil :: PushError( pLuaState, "'" + strPath + "' is not a directory" );

                lua_newtable( pLuaState );

                int  nIndex = 1;

                for( const fs :: directory_entry& entry :
                        fs :: recursive_directory_iterator( basePath, fs :: directory_options :: skip_permission_denied, errorCode ) )  {

                    if( !entry.is_regular_file( errorCode ) )
                        continue;

                    fs :: path     relativePath = fs :: relative( entry.path(), basePath, errorCode );
                    std :: string  strRelative = relativePath.generic_string();  // generic_string() always uses '/'

                    lua_pushlstring( pLuaState, strRelative.c_str(), strRelative.size() );
                    lua_rawseti( pLuaState, -2, nIndex++ );
                }

                return 1;
            }

            /**
             * @luaname{pack_read_file(path) -> data | nil, message}
             * @luagroup{pack_fs}
             * @luadoc
             * Reads the whole file at `path` (a real native OS path)
             * as raw bytes. Returns `nil, "message"` if it can't be
             * read.
             */
            int LuaPackApi :: ReadFile( lua_State* pLuaState )  {

                std :: string  strError;
                std :: string  strPath;

                if( !LuaEngineUtil :: ReadStringArg( pLuaState, 1, "path", strPath, strError ) )
                    return LuaEngineUtil :: PushError( pLuaState, strError );

                std :: ifstream  file( strPath, std :: ios :: binary | std :: ios :: ate );

                if( !file.is_open() )
                    return LuaEngineUtil :: PushError( pLuaState, "cannot open '" + strPath + "'" );

                std :: streamsize  nSize = file.tellg();

                file.seekg( 0, std :: ios :: beg );

                std :: vector<char>  data( ( size_t ) nSize );

                if( nSize > 0 && !file.read( data.data(), nSize ) )
                    return LuaEngineUtil :: PushError( pLuaState, "cannot read '" + strPath + "'" );

                lua_pushlstring( pLuaState, data.data(), data.size() );

                return 1;
            }

            /**
             * @luaname{pack_write_file(path, data) -> true | nil, message}
             * @luagroup{pack_fs}
             * @luadoc
             * Writes `data` (raw bytes) to `path` (a real native OS
             * path), creating any missing parent directories first.
             */
            int LuaPackApi :: WriteFile( lua_State* pLuaState )  {

                std :: string  strError;
                std :: string  strPath;
                std :: string  strData;

                if( !LuaEngineUtil :: ReadStringArg( pLuaState, 1, "path", strPath, strError ) ||
                    !LuaEngineUtil :: ReadStringArg( pLuaState, 2, "data", strData, strError ) )
                    return LuaEngineUtil :: PushError( pLuaState, strError );

                fs :: path         targetPath( strPath );
                std :: error_code  errorCode;

                if( targetPath.has_parent_path() )
                    fs :: create_directories( targetPath.parent_path(), errorCode );

                std :: ofstream  file( strPath, std :: ios :: binary | std :: ios :: trunc );

                if( !file.is_open() || !file.write( strData.data(), ( std :: streamsize ) strData.size() ) )
                    return LuaEngineUtil :: PushError( pLuaState, "cannot write '" + strPath + "'" );

                return LuaEngineUtil :: PushTrue( pLuaState );
            }

            /**
             * @luaname{pack_create_archive(path) -> handle | nil, message}
             * @luagroup{pack_archive}
             * @luaheading{Archive (miniz-backed .zip writing)}
             * @luadoc
             * Creates a new `.zip` archive at `path` (a real native OS
             * path, overwritten if it already exists), ready for
             * `pack_add_entry` calls. Returns `nil, "message"` on failure
             * (path not writable, etc) — it used to return `0`.
             */
            int LuaPackApi :: CreateArchive( lua_State* pLuaState )  {

                std :: string  strError;
                std :: string  strPath;

                if( !LuaEngineUtil :: ReadStringArg( pLuaState, 1, "path", strPath, strError ) )
                    return LuaEngineUtil :: PushError( pLuaState, strError );

                mz_zip_archive*  pZip = new mz_zip_archive();  // value-init zeroes it, same as memset - required by miniz's own contract

                if( !mz_zip_writer_init_file( pZip, strPath.c_str(), 0 ) )  {
                    delete pZip;

                    return LuaEngineUtil :: PushError( pLuaState, "cannot create archive '" + strPath + "'" );
                }

                int  nHandle = s_nNextHandle++;

                s_ArchiveHandles[nHandle] = pZip;

                lua_pushinteger( pLuaState, nHandle );

                return 1;
            }

            /**
             * @luaname{pack_add_entry(handle, entryName, data) -> true | nil, message}
             * @luagroup{pack_archive}
             * @luadoc
             * Adds `data` (raw bytes) to the archive `handle` (from
             * `pack_create_archive`) as a new entry named `entryName` -
             * typically one of `pack_list_directory`'s own returned
             * relative paths, after encrypting it's bytes via
             * `crypto_encrypt_data` (see LuaCryptoApi). Compressed at
             * miniz's own default level.
             */
            int LuaPackApi :: AddEntry( lua_State* pLuaState )  {

                std :: string  strError;
                std :: string  strEntry;
                std :: string  strData;
                int            nHandle = 0;

                if( !LuaEngineUtil :: ReadIntArg( pLuaState, 1, "handle", nHandle, strError ) ||
                    !LuaEngineUtil :: ReadStringArg( pLuaState, 2, "entryName", strEntry, strError ) ||
                    !LuaEngineUtil :: ReadStringArg( pLuaState, 3, "data", strData, strError ) )
                    return LuaEngineUtil :: PushError( pLuaState, strError );

                auto  it = s_ArchiveHandles.find( nHandle );

                if( it == s_ArchiveHandles.end() )
                    return LuaEngineUtil :: PushError( pLuaState, "invalid archive handle" );

                if( !mz_zip_writer_add_mem( it -> second, strEntry.c_str(), strData.data(), strData.size(), MZ_DEFAULT_COMPRESSION ) )
                    return LuaEngineUtil :: PushError( pLuaState, "cannot add '" + strEntry + "' to the archive" );

                return LuaEngineUtil :: PushTrue( pLuaState );
            }

            /**
             * @luaname{pack_close_archive(handle) -> true | nil, message}
             * @luagroup{pack_archive}
             * @luadoc
             * Finalizes and closes the archive `handle` - the `.zip`
             * file on disk isn't actually valid/complete until this is
             * called. The handle itself is no longer usable after this
             * call either way (success or failure).
             */
            int LuaPackApi :: CloseArchive( lua_State* pLuaState )  {

                std :: string  strError;
                int            nHandle = 0;

                if( !LuaEngineUtil :: ReadIntArg( pLuaState, 1, "handle", nHandle, strError ) )
                    return LuaEngineUtil :: PushError( pLuaState, strError );

                auto  it = s_ArchiveHandles.find( nHandle );

                if( it == s_ArchiveHandles.end() )
                    return LuaEngineUtil :: PushError( pLuaState, "invalid archive handle" );

                mz_zip_archive*  pZip    = it -> second;
                bool             bResult = mz_zip_writer_finalize_archive( pZip );

                mz_zip_writer_end( pZip );
                delete pZip;
                s_ArchiveHandles.erase( it );

                if( !bResult )
                    return LuaEngineUtil :: PushError( pLuaState, "cannot finalize the archive" );

                return LuaEngineUtil :: PushTrue( pLuaState );
            }

            /**
             * @brief Register the packaging-tool Lua-callable functions.
             */
            void LuaPackApi :: Register( lua_State* pLuaState )  {

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
