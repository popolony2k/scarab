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

#include "lua/luasaveapi.h"
#include "lua/luaappapi.h"
#include "lua/luaengineutil.h"
#include "lua/luajsonapi.h"
#include "engine/savedirectory.h"
#include "filesystem/filesystemfactory.h"
#include <cctype>
#include <vector>
#include <nlohmann/json.hpp>

#define __SAVE_NAME_EXTRA_CHARACTERS   "-_."     // besides letters and digits, in each part of a save's name
#define __SAVE_TEMP_SUFFIX             ".tmp"    // sunlight's IFileSystem::WriteFile writes "<target>.tmp" first
#define __SAVE_JSON_INDENT             2         // spaces per level of a written save
#define __HOST_PATH_SCHEME             "hpath://"


namespace Scarab  {
    namespace Engine  {
        namespace Lua  {

            /**
             * @brief Whether a text ends with a suffix, ignoring ASCII case (a save named "x.TMP" is
             * sunlight's temp file of "x" on a case-insensitive disk, as much as "x.tmp" is).
             *
             * @param strText The text;
             * @param szSuffix The suffix, in lower case;
             * @return true if the text ends with it;
             */
            static bool EndsWithIgnoringCase( const std :: string& strText, const char* szSuffix )  {

                std :: string  strSuffix( szSuffix );

                if( strText.size() < strSuffix.size() )
                    return false;

                for( size_t nPos = 0; nPos < strSuffix.size(); nPos++ )  {
                    unsigned char  cChar = static_cast<unsigned char>( strText[strText.size() - strSuffix.size() + nPos] );

                    if( std :: tolower( cChar ) != strSuffix[nPos] )
                        return false;
                }

                return true;
            }

            /**
             * @brief Whether a save's name is acceptable: a relative path of '/'-separated parts, each made
             * of letters, digits and "-_." only, none of them "." or "..", and not ending in ".tmp" (the
             * temp file sunlight writes beside a save). Such a name can never reach outside the save
             * directory, and holds no scheme ("hpath://" has a ':').
             *
             * @param strName The name;
             * @param strError Receives the reason when it is not acceptable (it starts "argument 'name'");
             * @return true if the name is acceptable;
             */
            bool LuaSaveApi :: IsValidSaveName( const std :: string& strName, std :: string& strError )  {

                if( strName.empty() )  {
                    strError = "argument 'name' can't be empty";
                    return false;
                }

                std :: string  strPart;
                size_t         nStart = 0;

                while( nStart <= strName.size() )  {
                    size_t  nSlash = strName.find( '/', nStart );

                    if( nSlash == std :: string :: npos )
                        nSlash = strName.size();

                    strPart = strName.substr( nStart, nSlash - nStart );

                    if( strPart.empty() )  {
                        strError = "argument 'name' must be a relative path like \"options.json\" or \"slots/1.json\" "
                                   "(no leading, trailing or doubled '/')";
                        return false;
                    }

                    if( ( strPart == "." ) || ( strPart == ".." ) )  {
                        strError = "argument 'name' can't have a '" + strPart + "' part";
                        return false;
                    }

                    for( char cChar : strPart )  {
                        if( !std :: isalnum( static_cast<unsigned char>( cChar ) ) &&
                            ( std :: string( __SAVE_NAME_EXTRA_CHARACTERS ).find( cChar ) == std :: string :: npos ) )  {
                            strError = "argument 'name' may only hold letters, digits, '-', '_', '.' and '/'";
                            return false;
                        }
                    }

                    nStart = nSlash + 1;
                }

                if( EndsWithIgnoringCase( strName, __SAVE_TEMP_SUFFIX ) )  {
                    strError = "argument 'name' can't end in '" __SAVE_TEMP_SUFFIX "' (the temporary file of a save being written)";
                    return false;
                }

                return true;
            }

            /**
             * @brief Read and check the save name argument (index 1).
             *
             * @param pLuaState Lua state to be used by engine call;
             * @param strName Receives the name;
             * @param strError Receives the reason on failure;
             * @return true if the argument is an acceptable save name;
             */
            static bool ReadSaveName( lua_State* pLuaState, std :: string& strName, std :: string& strError )  {

                return LuaEngineUtil :: ReadStringArg( pLuaState, 1, "name", strName, strError ) &&
                       LuaSaveApi :: IsValidSaveName( strName, strError );
            }

            /**
             * @brief The real path of a save inside the save directory, as a host path Scarab builds itself
             * (never one taken from Lua): reading through it finds only the save, never a file of the same
             * name in another mount (a loose run's working folder is mounted at the root too).
             *
             * @param pLuaState Lua state to be used by engine call;
             * @param strName The save's name (already checked);
             * @param strHostPath Receives "hpath://<save folder>/<name>";
             * @param strError Receives the reason on failure;
             * @return true if there is a save directory;
             */
            static bool SaveHostPath( lua_State* pLuaState, const std :: string& strName, std :: string& strHostPath, std :: string& strError )  {

                Engine :: SaveDirectory*  pSaveDirectory = static_cast<Engine :: SaveDirectory *>(
                                              LuaEngineUtil :: GetGlobalPointer( pLuaState, LuaAppApi :: SAVE_DIRECTORY_GLOBAL ) );

                if( pSaveDirectory -> GetPath().empty() )  {
                    strError = "no save directory (app_set_save_directory, or \"save_directory\" in the project file)";
                    return false;
                }

                strHostPath = __HOST_PATH_SCHEME + pSaveDirectory -> GetPath() + "/" + strName;

                return true;
            }

            /**
             * @brief Write a Lua table as a save, atomically (sunlight's IFileSystem::WriteFile: a temp file,
             * synced, renamed over the old save). The virtual path goes into the write directory only.
             *
             * @param pLuaState Lua state to be used by engine call;
             * @return int number of return data (if any - required by lua engine)
             *
             * @luaname{save_write(name, value) -> true | nil, message}
             * @luadoc
             * Write `value` (a table) as the save `name`, as pretty-printed JSON with its keys sorted.
             * The old save, if any, is replaced only once the new one is safely on the disk. A
             * subfolder in the name (`"slots/1.json"`) is created when needed. Refused, with where in
             * the table: anything JSON can't hold exactly - mixed integer and string keys, an array with
             * holes, a key that is not a string or an integer `1..n`, a function, userdata or thread, a
             * table that contains itself, more than 64 tables deep, NaN or infinity, text that is not
             * UTF-8 (eg. `"argument 'value' can't be saved: value.scores[3] is a function"`).
             * @luaexample
             * local ok, err = save_write( "highscores.json", { { name = "AAA", score = 3000 } } )
             *
             * if not ok then print( err ) end
             */
            int LuaSaveApi :: Write( lua_State* pLuaState )  {

                std :: string      strError;
                std :: string      strName;
                std :: string      strHostPath;
                nlohmann :: json   value;

                if( !ReadSaveName( pLuaState, strName, strError ) ||
                    !LuaEngineUtil :: CheckTableArg( pLuaState, 2, "value", strError ) )
                    return LuaEngineUtil :: PushError( pLuaState, strError );

                if( !LuaJsonApi :: ToJson( pLuaState, 2, "value", value, strError ) )
                    return LuaEngineUtil :: PushError( pLuaState, "argument 'value' can't be saved: " + strError );

                if( !SaveHostPath( pLuaState, strName, strHostPath, strError ) )
                    return LuaEngineUtil :: PushError( pLuaState, "cannot write save '" + strName + "': " + strError );

                std :: string                 strText = value.dump( __SAVE_JSON_INDENT ) + "\n";
                std :: vector<unsigned char>  data( strText.begin(), strText.end() );

                if( !SunLight :: FileSystem :: FileSystemFactory :: GetFileSystem().WriteFile( strName, data ) )
                    return LuaEngineUtil :: PushError( pLuaState, "cannot write save '" + strName + "'" );

                return LuaEngineUtil :: PushTrue( pLuaState );
            }

            /**
             * @brief Read a save back as a Lua table, from the save directory only (through a host path
             * Scarab builds, so a shipped file of the same name is never taken for a save).
             *
             * @param pLuaState Lua state to be used by engine call;
             * @return int number of return data (if any - required by lua engine)
             *
             * @luaname{save_read(name) -> table | nil, message}
             * @luadoc
             * Read the save `name` back as the table `save_write` was given: integers as integers,
             * floats as floats. Only the save directory is read - a file of the same name shipped with
             * the game is never returned. `nil, "no save named '<name>'"` when there is none, so a
             * game can fall back to its defaults; a file that is not a JSON object or array (eg. edited
             * by hand) is `nil, "cannot parse save ..."`.
             * @luaexample
             * local options = save_read( "options.json" ) or { volume = 8 }
             */
            int LuaSaveApi :: Read( lua_State* pLuaState )  {

                std :: string                 strError;
                std :: string                 strName;
                std :: string                 strHostPath;
                std :: vector<unsigned char>  data;
                nlohmann :: json              value;

                if( !ReadSaveName( pLuaState, strName, strError ) )
                    return LuaEngineUtil :: PushError( pLuaState, strError );

                if( !SaveHostPath( pLuaState, strName, strHostPath, strError ) )
                    return LuaEngineUtil :: PushError( pLuaState, "cannot read save '" + strName + "': " + strError );

                SunLight :: FileSystem :: IFileSystem&  fileSystem = SunLight :: FileSystem :: FileSystemFactory :: GetFileSystem();

                if( !fileSystem.ReadFile( strHostPath, data ) )
                    return LuaEngineUtil :: PushError( pLuaState, fileSystem.Exists( strHostPath ) ? "cannot read save '" + strName + "'"
                                                                                                  : "no save named '" + strName + "'" );

                try  {
                    value = nlohmann :: json :: parse( data.begin(), data.end() );
                }
                catch( const nlohmann :: json :: parse_error& ex )  {
                    return LuaEngineUtil :: PushError( pLuaState, "cannot parse save '" + strName + "': " + ex.what() );
                }

                if( !value.is_structured() )
                    return LuaEngineUtil :: PushError( pLuaState, "cannot parse save '" + strName + "': not a JSON object or array" );

                if( !LuaJsonApi :: PushJsonValue( pLuaState, value, strError ) )
                    return LuaEngineUtil :: PushError( pLuaState, "cannot read save '" + strName + "': " + strError );

                return 1;
            }

            /**
             * @brief Whether a save exists: a FILE of that name in the save directory (a folder of that
             * name is not a save - sunlight's Exists on a host path checks for a regular file).
             *
             * @param pLuaState Lua state to be used by engine call;
             * @return int number of return data (if any - required by lua engine)
             *
             * @luaname{save_exists(name) -> boolean | nil, message}
             * @luadoc
             * `true` if the save `name` exists in the save directory, `false` if not (a folder of that
             * name is not a save). `nil, "message"` only for a bad name or no save directory.
             * @luaexample
             * if save_exists( "slots/1.json" ) then print( "slot 1 is taken" ) end
             */
            int LuaSaveApi :: Exists( lua_State* pLuaState )  {

                std :: string  strError;
                std :: string  strName;
                std :: string  strHostPath;

                if( !ReadSaveName( pLuaState, strName, strError ) )
                    return LuaEngineUtil :: PushError( pLuaState, strError );

                if( !SaveHostPath( pLuaState, strName, strHostPath, strError ) )
                    return LuaEngineUtil :: PushError( pLuaState, "cannot look for save '" + strName + "': " + strError );

                lua_pushboolean( pLuaState, SunLight :: FileSystem :: FileSystemFactory :: GetFileSystem().Exists( strHostPath ) );

                return 1;
            }

            /**
             * @brief Delete a save (sunlight's IFileSystem::RemoveFile on the virtual path: only inside the
             * write directory, files only - a shipped file of the same name is never touched).
             *
             * @param pLuaState Lua state to be used by engine call;
             * @return int number of return data (if any - required by lua engine)
             *
             * @luaname{save_delete(name) -> true | nil, message}
             * @luadoc
             * Delete the save `name`. `nil, "no save named '<name>'"` when there is none. Only the save
             * is deleted - never a file shipped with the game, and never a folder.
             * @luaexample
             * save_delete( "options.json" )     -- back to the defaults next time
             */
            int LuaSaveApi :: Delete( lua_State* pLuaState )  {

                std :: string  strError;
                std :: string  strName;
                std :: string  strHostPath;

                if( !ReadSaveName( pLuaState, strName, strError ) )
                    return LuaEngineUtil :: PushError( pLuaState, strError );

                if( !SaveHostPath( pLuaState, strName, strHostPath, strError ) )
                    return LuaEngineUtil :: PushError( pLuaState, "cannot delete save '" + strName + "': " + strError );

                SunLight :: FileSystem :: IFileSystem&  fileSystem = SunLight :: FileSystem :: FileSystemFactory :: GetFileSystem();

                if( !fileSystem.Exists( strHostPath ) )
                    return LuaEngineUtil :: PushError( pLuaState, "no save named '" + strName + "'" );

                if( !fileSystem.RemoveFile( strName ) )
                    return LuaEngineUtil :: PushError( pLuaState, "cannot delete save '" + strName + "'" );

                return LuaEngineUtil :: PushTrue( pLuaState );
            }

            /**
             * @brief Register the save functions. They need no renderer.
             *
             * @param pLuaState Lua state to be used by engine call;
             */
            void LuaSaveApi :: Register( lua_State* pLuaState )  {

                lua_register( pLuaState, "save_write", LuaSaveApi :: Write );
                lua_register( pLuaState, "save_read", LuaSaveApi :: Read );
                lua_register( pLuaState, "save_exists", LuaSaveApi :: Exists );
                lua_register( pLuaState, "save_delete", LuaSaveApi :: Delete );
            }
        }
    }
}
