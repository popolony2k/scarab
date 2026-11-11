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

#include "lua/luasandboxapi.h"
#include "lua/luaappapi.h"
#include "lua/luaengineutil.h"
#include "lua/luafilesystemapi.h"
#include "engine/sandboxfile.h"
#include "engine/savedirectory.h"
#include "filesystem/filepath.h"
#include "filesystem/filesystemfactory.h"
#include <cerrno>
#include <cstdio>
#include <cstring>
#include <new>
#include <random>
#include <string>
#include <vector>

extern "C"  {
  #include "lauxlib.h"
  #include "lualib.h"
}

#define __FILE_METATABLE       "scarab.SandboxFILE*"     // the metatable of the sandbox's files
#define __DEFAULT_INPUT_KEY    "scarab.io.input"         // registry: io.input()'s file
#define __DEFAULT_OUTPUT_KEY   "scarab.io.output"        // registry: io.output()'s file
#define __INSTALLED_KEY        "scarab.sandbox"          // registry: true once the sandbox is installed
#define __APP_DIRECTORY_KEY    "scarab.sandbox.appdir"   // registry: APP_DIR, as the engine computed it
#define __NOT_AVAILABLE        " is not available in the sandbox"
#define __NO_SAVE_DIRECTORY    "no save directory (app_set_save_directory, \"save_directory\" in the project file, or --save-dir)"
#define __MAX_LINES_FORMATS    250                       // Lua's MAXARGLINE (liolib.c)
#define __NUMBER_BUFFER_SIZE   64                        // a formatted lua_Integer or lua_Number fits
#define __TMPNAME_PREFIX       "lua_"
#define __TMPNAME_LETTERS      "abcdefghijklmnopqrstuvwxyz0123456789"
#define __TMPNAME_LENGTH       6                         // random letters after the prefix
#define __TMPNAME_TRIES        100                       // names tried before giving up
#define __TEXT_MODE            "t"                       // a chunk mode: text only
#define __ANY_MODE             "bt"                      // Lua's default chunk mode
#define __PATH_SEPARATOR       ';'                       // between package.path's templates (LUA_PATH_SEP)
#define __PATH_MARK            '?'                       // replaced by a module's name (LUA_PATH_MARK)
#define __VIRTUAL_SEPARATOR    "/"                       // a virtual path's folder separator, on every OS
#define __CHUNK_RESERVED_SLOT  5                         // load's reader keeps its string here (lbaselib.c)
#define __LOADFILE_ENV_ARG     3                         // loadfile( filename, mode, env ): env's position
#define __LOAD_ENV_ARG         4                         // load( chunk, chunkname, mode, env ): env's position


namespace Scarab  {
    namespace Engine  {
        namespace Lua  {

            /** @brief A sandbox file, as a Lua full userdata holds it. */
            struct stLuaFile  {
                Engine :: SandboxFile  file;
                std :: string          strSavePath;     // where it is written back, "" for nowhere (read-only, io.tmpfile)
                bool                   bClosed;
            };

            /**
             * @brief Push Lua's failure triple for a file operation (luaL_fileresult): nil, "<name>: <reason>"
             * (or the reason alone with no name), and an errno-style code.
             *
             * @param pLuaState Lua state to be used by engine call;
             * @param szName The file's name, or nullptr;
             * @param nErrorCode The errno-style code (its strerror is the reason when strReason is empty);
             * @param strReason The reason, "" for the code's own;
             * @return 3, the number of values pushed;
             */
            static int PushFailure( lua_State* pLuaState, const char* szName, int nErrorCode, const std :: string& strReason = std :: string() )  {

                const char*  szReason = strReason.empty() ? std :: strerror( nErrorCode ) : strReason.c_str();

                luaL_pushfail( pLuaState );

                if( szName )
                    lua_pushfstring( pLuaState, "%s: %s", szName, szReason );
                else
                    lua_pushstring( pLuaState, szReason );

                lua_pushinteger( pLuaState, nErrorCode );

                return 3;
            }

            /**
             * @brief The game's save directory, as LuaEngine::Init stashed it (nullptr before Init).
             *
             * @param pLuaState Lua state to be used by engine call;
             * @return The save directory, or nullptr;
             */
            static Engine :: SaveDirectory* GetSaveDirectory( lua_State* pLuaState )  {

                return static_cast<Engine :: SaveDirectory *>( LuaEngineUtil :: GetEnginePointer( pLuaState, LuaAppApi :: SAVE_DIRECTORY_KEY ) );
            }

            /**
             * @brief Where a path is written: inside the save directory (LuaSandboxApi::ToSavePath), which must
             * exist.
             *
             * @param pLuaState Lua state to be used by engine call;
             * @param szPath The path a script gave;
             * @param strSavePath Receives the path inside the save directory;
             * @param strError Receives the reason on failure;
             * @return true if the path can be written;
             */
            static bool ResolveWrite( lua_State* pLuaState, const char* szPath, std :: string& strSavePath, std :: string& strError )  {

                Engine :: SaveDirectory*  pSaveDirectory = GetSaveDirectory( pLuaState );

                if( !pSaveDirectory || pSaveDirectory -> GetPath().empty() )  {
                    strError = __NO_SAVE_DIRECTORY;
                    return false;
                }

                lua_getfield( pLuaState, LUA_REGISTRYINDEX, __APP_DIRECTORY_KEY );

                std :: string  strAppDirectory = lua_isstring( pLuaState, -1 ) ? lua_tostring( pLuaState, -1 ) : "";

                lua_pop( pLuaState, 1 );

                return LuaSandboxApi :: ToSavePath( szPath, strAppDirectory, strSavePath, strError );
            }

            /**
             * @brief Whether a file is in the save directory itself - read through the save folder's private
             * mount, which sees nothing else (never a game file of the same name).
             *
             * @param strSavePath The path inside the save directory;
             * @param data Receives the file's bytes;
             * @return true if the save directory holds that file;
             */
            static bool ReadSaveFile( const std :: string& strSavePath, std :: vector<unsigned char>& data )  {

                return SunLight :: FileSystem :: FileSystemFactory :: GetFileSystem().ReadFile( Engine :: SaveDirectory :: GetSavePath( strSavePath ), data );
            }

            /**
             * @brief Write a sandbox file back into the save directory if it changed (atomically: sunlight's
             * IFileSystem::WriteFile). A file with nowhere to go (read-only, io.tmpfile) has nothing to write.
             *
             * @param pFile The file;
             * @param strError Receives the reason on failure;
             * @return true if nothing was left unwritten;
             */
            static bool WriteBack( stLuaFile* pFile, std :: string& strError )  {

                if( pFile -> strSavePath.empty() || !pFile -> file.IsDirty() )
                    return true;

                const std :: vector<char>&     contents = pFile -> file.GetData();
                std :: vector<unsigned char>  data( contents.begin(), contents.end() );

                if( !SunLight :: FileSystem :: FileSystemFactory :: GetFileSystem().WriteFile( pFile -> strSavePath, data ) )  {
                    strError = "cannot write '" + pFile -> strSavePath + "' into the save directory";
                    return false;
                }

                pFile -> file.MarkClean();

                return true;
            }

            /**
             * @brief Push a new, open sandbox file.
             *
             * @param pLuaState Lua state to be used by engine call;
             * @param mode Its mode;
             * @param existing The contents it starts with;
             * @param strSavePath Where it is written back ("" for nowhere);
             * @return The file;
             */
            static stLuaFile* PushNewFile( lua_State* pLuaState, const Engine :: SandboxFile :: stMode& mode,
                                           const std :: vector<char>& existing, const std :: string& strSavePath )  {

                void*       pMemory = lua_newuserdatauv( pLuaState, sizeof( stLuaFile ), 0 );
                stLuaFile*  pFile   = new( pMemory ) stLuaFile();

                pFile -> bClosed     = false;
                pFile -> strSavePath = strSavePath;
                pFile -> file.Open( mode, existing );
                luaL_setmetatable( pLuaState, __FILE_METATABLE );

                return pFile;
            }

            /**
             * @brief Open a file by the path rule and push it: reading looks in the save directory, then the
             * game's own files (the file system's mounts); a writing mode writes into the save directory. Nothing
             * is pushed on failure.
             *
             * @param pLuaState Lua state to be used by engine call;
             * @param szName The path a script gave;
             * @param mode The mode;
             * @param nErrorCode Receives an errno-style code on failure;
             * @param strReason Receives the reason on failure ("" for the code's own);
             * @return true if the file was pushed;
             */
            static bool TryOpen( lua_State* pLuaState, const char* szName, const Engine :: SandboxFile :: stMode& mode,
                                 int& nErrorCode, std :: string& strReason )  {

                std :: string  strRest;

                switch( SunLight :: FileSystem :: FilePath :: SplitPath( szName, strRest ) )  {
                    case SunLight :: FileSystem :: FilePath :: PATH_SCHEME_VIRTUAL :
                        break;
                    case SunLight :: FileSystem :: FilePath :: PATH_SCHEME_HOST :
                        nErrorCode = EACCES;
                        strReason  = "a host path can't be opened in the sandbox - only the game's own files and its save directory";
                        return false;
                    default :
                        nErrorCode = EACCES;
                        strReason  = "unknown path scheme";
                        return false;
                }

                std :: vector<char>  existing;
                bool                 bExists = false;

                if( !mode.bTruncate )  {
                    std :: vector<unsigned char>  data;

                    bExists = SunLight :: FileSystem :: FileSystemFactory :: GetFileSystem().ReadFile( szName, data );
                    existing.assign( data.begin(), data.end() );
                }

                if( mode.bMustExist && !bExists )  {
                    nErrorCode = ENOENT;
                    return false;
                }

                std :: string  strSavePath;

                if( mode.bWrite && !ResolveWrite( pLuaState, szName, strSavePath, strReason ) )  {
                    nErrorCode = EACCES;
                    return false;
                }

                stLuaFile*  pFile = PushNewFile( pLuaState, mode, existing, strSavePath );

                // A new file in a writing mode exists once closed, empty or not - as Lua's creates it on open.
                if( mode.bWrite && !bExists )
                    pFile -> file.Write( "", 0 );

                return true;
            }

            /**
             * @brief Open a file and push it, or push Lua's own message for a failed open (liolib.c's opencheck):
             * OpenOrRaise's work, in a frame of its own so the error is raised where no C++ object lives.
             *
             * @param pLuaState Lua state to be used by engine call;
             * @param szName The path a script gave;
             * @param szMode A valid Lua mode string;
             * @return true if the file was pushed, false if the message was;
             */
            static bool TryOpenOrMessage( lua_State* pLuaState, const char* szName, const char* szMode )  {

                Engine :: SandboxFile :: stMode  mode;
                int                              nErrorCode = 0;
                std :: string                    strReason;

                Engine :: SandboxFile :: ParseMode( szMode, mode );

                if( TryOpen( pLuaState, szName, mode, nErrorCode, strReason ) )
                    return true;

                lua_pushfstring( pLuaState, "cannot open file '%s' (%s)", szName,
                                 strReason.empty() ? std :: strerror( nErrorCode ) : strReason.c_str() );

                return false;
            }

            /**
             * @brief Open a file or raise Lua's own error for it (io.lines, io.input and io.output raise rather
             * than return nil). Lua is built as C, so an error is a longjmp: this frame holds no C++ object
             * (TryOpenOrMessage does the work) - on Windows, unwinding through one crashed (C8, found in CI).
             *
             * @param pLuaState Lua state to be used by engine call;
             * @param szName The path a script gave;
             * @param szMode A valid Lua mode string;
             */
            static void OpenOrRaise( lua_State* pLuaState, const char* szName, const char* szMode )  {

                if( !TryOpenOrMessage( pLuaState, szName, szMode ) )
                    lua_error( pLuaState );
            }

            /**
             * @brief The sandbox file at a stack index, open - or Lua's own error ("attempt to use a closed file",
             * or a wrong-type argument).
             *
             * @param pLuaState Lua state to be used by engine call;
             * @param nIndex Its stack index;
             * @return The file;
             */
            static stLuaFile* CheckOpenFile( lua_State* pLuaState, int nIndex )  {

                stLuaFile*  pFile = static_cast<stLuaFile *>( luaL_checkudata( pLuaState, nIndex, __FILE_METATABLE ) );

                if( pFile -> bClosed )
                    luaL_error( pLuaState, "attempt to use a closed file" );

                return pFile;
            }

            /**
             * @brief Check that a stack index holds an open file, the sandbox's or one of Lua's own (io.stdout
             * and the like) - Lua's own errors otherwise.
             *
             * @param pLuaState Lua state to be used by engine call;
             * @param nIndex Its stack index;
             */
            static void CheckAnyOpenFile( lua_State* pLuaState, int nIndex )  {

                if( luaL_testudata( pLuaState, nIndex, __FILE_METATABLE ) )  {
                    CheckOpenFile( pLuaState, nIndex );
                    return;
                }

                luaL_Stream*  pStream = static_cast<luaL_Stream *>( luaL_checkudata( pLuaState, nIndex, LUA_FILEHANDLE ) );

                if( !pStream -> closef )
                    luaL_error( pLuaState, "attempt to use a closed file" );
            }

            /**
             * @brief Call a method of the object at index 1 with the arguments after it, and return what it
             * returns: how io.read, io.write, io.close and io.lines() reach the default file, Lua's own or the
             * sandbox's alike.
             *
             * @param pLuaState Lua state to be used by engine call;
             * @param szMethod The method's name;
             * @return The number of values it returned;
             */
            static int CallFileMethod( lua_State* pLuaState, const char* szMethod )  {

                int  nArgs = lua_gettop( pLuaState ) - 1;

                CheckAnyOpenFile( pLuaState, 1 );
                luaL_getmetafield( pLuaState, 1, "__index" );
                lua_getfield( pLuaState, -1, szMethod );
                lua_remove( pLuaState, -2 );
                lua_insert( pLuaState, 1 );
                lua_call( pLuaState, nArgs + 1, LUA_MULTRET );

                return lua_gettop( pLuaState );
            }

            /**
             * @brief Push a line ("l", or "L" keeping the newline).
             *
             * @param pLuaState Lua state to be used by engine call;
             * @param pFile The file;
             * @param bKeepNewline Whether the newline is kept;
             * @return false at the end of the file;
             */
            static bool PushLine( lua_State* pLuaState, stLuaFile* pFile, bool bKeepNewline )  {

                std :: string  strLine;
                bool           bRead = pFile -> file.ReadLine( bKeepNewline, strLine );

                lua_pushlstring( pLuaState, strLine.data(), strLine.size() );

                return bRead;
            }

            /**
             * @brief Push up to nCount bytes (Lua's numeric format; 0 tests for the end of the file).
             *
             * @param pLuaState Lua state to be used by engine call;
             * @param pFile The file;
             * @param nCount How many bytes;
             * @return false at the end of the file;
             */
            static bool PushCount( lua_State* pLuaState, stLuaFile* pFile, std :: size_t nCount )  {

                std :: string  strBytes;
                bool           bRead = pFile -> file.ReadCount( nCount, strBytes );

                lua_pushlstring( pLuaState, strBytes.data(), strBytes.size() );

                return bRead;
            }

            /**
             * @brief Push a number read as Lua's "n" reads it, or nil when the text is not one.
             *
             * @param pLuaState Lua state to be used by engine call;
             * @param pFile The file;
             * @return false when no number was read;
             */
            static bool PushNumber( lua_State* pLuaState, stLuaFile* pFile )  {

                std :: string  strNumeral = pFile -> file.ReadNumberText();

                if( lua_stringtonumber( pLuaState, strNumeral.c_str() ) != 0 )
                    return true;

                lua_pushnil( pLuaState );

                return false;
            }

            /**
             * @brief Push the rest of the file ("a").
             *
             * @param pLuaState Lua state to be used by engine call;
             * @param pFile The file;
             */
            static void PushAll( lua_State* pLuaState, stLuaFile* pFile )  {

                std :: string  strRest = pFile -> file.ReadAll();

                lua_pushlstring( pLuaState, strRest.data(), strRest.size() );
            }

            /**
             * @brief Read by the formats from a stack index on, as Lua's g_read does (liolib.c): "l" by default;
             * every value read is pushed, and reading stops at the first one that fails, which is pushed as nil.
             *
             * @param pLuaState Lua state to be used by engine call;
             * @param pFile The file;
             * @param nFirst The stack index of the first format;
             * @return The number of values pushed;
             */
            static int ReadFormats( lua_State* pLuaState, stLuaFile* pFile, int nFirst )  {

                if( !pFile -> file.CanRead() )
                    return PushFailure( pLuaState, nullptr, EBADF );

                int   nArgs = lua_gettop( pLuaState ) - nFirst + 1;
                int   n;
                bool  bSuccess = true;

                if( nArgs == 0 )  {
                    bSuccess = PushLine( pLuaState, pFile, false );
                    n        = nFirst + 1;
                }
                else  {
                    luaL_checkstack( pLuaState, nArgs + LUA_MINSTACK, "too many arguments" );

                    for( n = nFirst; ( nArgs-- > 0 ) && bSuccess; n++ )  {
                        if( lua_type( pLuaState, n ) == LUA_TNUMBER )  {
                            std :: size_t  nCount = ( std :: size_t ) luaL_checkinteger( pLuaState, n );

                            bSuccess = PushCount( pLuaState, pFile, nCount );
                            continue;
                        }

                        const char*  szFormat = luaL_checkstring( pLuaState, n );

                        if( *szFormat == '*' )      // Lua 5.4 still accepts "*l" and the like
                            szFormat++;

                        switch( *szFormat )  {
                            case 'n' :
                                bSuccess = PushNumber( pLuaState, pFile );
                                break;
                            case 'l' :
                                bSuccess = PushLine( pLuaState, pFile, false );
                                break;
                            case 'L' :
                                bSuccess = PushLine( pLuaState, pFile, true );
                                break;
                            case 'a' :
                                PushAll( pLuaState, pFile );
                                break;
                            default :
                                return luaL_argerror( pLuaState, n, "invalid format" );
                        }
                    }
                }

                if( !bSuccess )  {
                    lua_pop( pLuaState, 1 );
                    luaL_pushfail( pLuaState );
                }

                return n - nFirst;
            }

            /**
             * @brief Push the iterator of file:lines / io.lines (liolib.c's aux_lines): a closure over the file
             * (index 1), the number of formats, whether it closes the file at the end, and the formats.
             *
             * @param pLuaState Lua state to be used by engine call;
             * @param bCloseAtEnd Whether the iterator closes the file once it is read;
             * @param fnReadLine The iterator's function;
             */
            static void PushLinesIterator( lua_State* pLuaState, bool bCloseAtEnd, lua_CFunction fnReadLine )  {

                int  nFormats = lua_gettop( pLuaState ) - 1;

                luaL_argcheck( pLuaState, nFormats <= __MAX_LINES_FORMATS, __MAX_LINES_FORMATS + 2, "too many arguments" );
                lua_pushvalue( pLuaState, 1 );
                lua_pushinteger( pLuaState, nFormats );
                lua_pushboolean( pLuaState, bCloseAtEnd );
                lua_rotate( pLuaState, 2, 3 );
                lua_pushcclosure( pLuaState, fnReadLine, 3 + nFormats );
            }

            /**
             * @brief Finish a file that is going away (collected, or a to-be-closed variable's end): write it
             * back if it changed, ignoring a failure - there is no one to report it to (Lua's own does the same).
             *
             * @param pFile The file;
             */
            static void Finish( stLuaFile* pFile )  {

                if( pFile -> bClosed )
                    return;

                std :: string  strIgnored;

                WriteBack( pFile, strIgnored );
                pFile -> bClosed = true;
            }

            /**
             * @brief The result of loading a chunk, as Lua's load and loadfile return it (lbaselib.c's load_aux):
             * the function, with env as its first upvalue when one was given, or nil and the message.
             *
             * @param pLuaState Lua state to be used by engine call;
             * @param nStatus The load's status;
             * @param nEnvIndex The stack index of env, 0 for none;
             * @return The number of values pushed;
             */
            static int LoadResult( lua_State* pLuaState, int nStatus, int nEnvIndex )  {

                if( nStatus == LUA_OK )  {
                    if( nEnvIndex != 0 )  {
                        lua_pushvalue( pLuaState, nEnvIndex );

                        if( !lua_setupvalue( pLuaState, -2, 1 ) )
                            lua_pop( pLuaState, 1 );
                    }

                    return 1;
                }

                luaL_pushfail( pLuaState );
                lua_insert( pLuaState, -2 );

                return 2;
            }

            /**
             * @brief Push nil and the sandbox's refusal of a binary chunk, for a load mode without "t".
             *
             * @param pLuaState Lua state to be used by engine call;
             * @param szMode The mode asked for;
             * @return 2, the number of values pushed;
             */
            static int PushBinaryRefused( lua_State* pLuaState, const char* szMode )  {

                luaL_pushfail( pLuaState );
                lua_pushfstring( pLuaState, "binary chunks are not available in the sandbox (mode '%s')", szMode );

                return 2;
            }

            /**
             * @brief load's reader for a function chunk (lbaselib.c's generic_reader): calls the function for
             * each piece, keeping the string in a reserved stack slot while Lua parses it.
             *
             * @param pLuaState Lua state to be used by engine call;
             * @param pUserData Unused;
             * @param pSize Receives the piece's size;
             * @return The piece, or nullptr at the end;
             */
            static const char* ReadChunkPiece( lua_State* pLuaState, void* pUserData, std :: size_t* pSize )  {

                ( void ) pUserData;

                luaL_checkstack( pLuaState, 2, "too many nested functions" );
                lua_pushvalue( pLuaState, 1 );
                lua_call( pLuaState, 0, 1 );

                if( lua_isnil( pLuaState, -1 ) )  {
                    lua_pop( pLuaState, 1 );
                    *pSize = 0;
                    return nullptr;
                }

                if( !lua_isstring( pLuaState, -1 ) )
                    luaL_error( pLuaState, "reader function must return a string" );

                lua_replace( pLuaState, __CHUNK_RESERVED_SLOT );

                return lua_tolstring( pLuaState, __CHUNK_RESERVED_SLOT, pSize );
            }

            /**
             * @brief Find a module's file in a package.path-style list of templates, as Lua's searchpath does
             * (loadlib.c) - but through SunLight::FileSystem, so by the path rule.
             *
             * @param strName The module's name;
             * @param strPath The templates, separated by ';', '?' standing for the name;
             * @param strSeparator Replaced in the name by strDirSeparator ("" for none);
             * @param strDirSeparator What replaces it;
             * @param strFound Receives the file found;
             * @param strTried Receives Lua's "no file '...'" list when none is found;
             * @return true if a file was found;
             */
            static bool FindInPath( const std :: string& strName, const std :: string& strPath, const std :: string& strSeparator,
                                    const std :: string& strDirSeparator, std :: string& strFound, std :: string& strTried )  {

                std :: string  strModule = strName;

                if( !strSeparator.empty() )  {
                    for( std :: size_t nPos = strModule.find( strSeparator ); nPos != std :: string :: npos;
                         nPos = strModule.find( strSeparator, nPos + strDirSeparator.size() ) )
                        strModule.replace( nPos, strSeparator.size(), strDirSeparator );
                }

                strTried.clear();

                std :: size_t  nStart = 0;

                while( nStart <= strPath.size() )  {
                    std :: size_t  nEnd = strPath.find( __PATH_SEPARATOR, nStart );

                    if( nEnd == std :: string :: npos )
                        nEnd = strPath.size();

                    std :: string  strFile = strPath.substr( nStart, nEnd - nStart );

                    for( std :: size_t nPos = strFile.find( __PATH_MARK ); nPos != std :: string :: npos;
                         nPos = strFile.find( __PATH_MARK, nPos + strModule.size() ) )
                        strFile.replace( nPos, 1, strModule );

                    if( !strFile.empty() && SunLight :: FileSystem :: FileSystemFactory :: GetFileSystem().Exists( strFile ) )  {
                        strFound = strFile;
                        return true;
                    }

                    strTried += ( strTried.empty() ? "" : "\n\t" ) + std :: string( "no file '" ) + strFile + "'";
                    nStart = nEnd + 1;
                }

                return false;
            }

            /**
             * @brief io.open( filename [, mode] ): a sandbox file, by the path rule.
             *
             * @param pLuaState Lua state to be used by engine call;
             * @return The file, or nil, a message and a code;
             */
            int LuaSandboxApi :: IoOpen( lua_State* pLuaState )  {

                const char*                      szName = luaL_checkstring( pLuaState, 1 );
                const char*                      szMode = luaL_optstring( pLuaState, 2, "r" );
                Engine :: SandboxFile :: stMode  mode;

                luaL_argcheck( pLuaState, Engine :: SandboxFile :: ParseMode( szMode, mode ), 2, "invalid mode" );

                int            nErrorCode = 0;
                std :: string  strReason;

                if( !TryOpen( pLuaState, szName, mode, nErrorCode, strReason ) )
                    return PushFailure( pLuaState, szName, nErrorCode, strReason );

                return 1;
            }

            /**
             * @brief io.lines( [filename, ...] ): the lines of a file (closed at the end), or of the default
             * input (left open).
             *
             * @param pLuaState Lua state to be used by engine call;
             * @return The iterator - and, for a file, nil, nil and the file to close (Lua 5.4's four values);
             */
            int LuaSandboxApi :: IoLines( lua_State* pLuaState )  {

                if( lua_isnone( pLuaState, 1 ) )
                    lua_pushnil( pLuaState );

                if( lua_isnil( pLuaState, 1 ) )  {
                    lua_getfield( pLuaState, LUA_REGISTRYINDEX, __DEFAULT_INPUT_KEY );
                    lua_replace( pLuaState, 1 );

                    return CallFileMethod( pLuaState, "lines" );
                }

                const char*  szName = luaL_checkstring( pLuaState, 1 );

                OpenOrRaise( pLuaState, szName, "r" );
                lua_replace( pLuaState, 1 );
                PushLinesIterator( pLuaState, true, LuaSandboxApi :: FileReadLine );
                lua_pushnil( pLuaState );
                lua_pushnil( pLuaState );
                lua_pushvalue( pLuaState, 1 );

                return 4;
            }

            /**
             * @brief io.input / io.output's shared body (liolib.c's g_iofile): a name opens a file, a file is
             * used as it is; either becomes the default. Returns the default.
             *
             * @param pLuaState Lua state to be used by engine call;
             * @param szKey The registry key of the default;
             * @param szMode The mode a name is opened in;
             * @return 1, the default file;
             */
            static int SetDefaultFile( lua_State* pLuaState, const char* szKey, const char* szMode )  {

                if( !lua_isnoneornil( pLuaState, 1 ) )  {
                    const char*  szName = lua_tostring( pLuaState, 1 );

                    if( szName )
                        OpenOrRaise( pLuaState, szName, szMode );
                    else  {
                        CheckAnyOpenFile( pLuaState, 1 );
                        lua_pushvalue( pLuaState, 1 );
                    }

                    lua_setfield( pLuaState, LUA_REGISTRYINDEX, szKey );
                }

                lua_getfield( pLuaState, LUA_REGISTRYINDEX, szKey );

                return 1;
            }

            /**
             * @brief io.input( [file] ): the default input (a name is opened for reading, by the path rule).
             *
             * @param pLuaState Lua state to be used by engine call;
             * @return 1, the default input;
             */
            int LuaSandboxApi :: IoInput( lua_State* pLuaState )  {

                return SetDefaultFile( pLuaState, __DEFAULT_INPUT_KEY, "r" );
            }

            /**
             * @brief io.output( [file] ): the default output (a name is opened for writing, in the save
             * directory).
             *
             * @param pLuaState Lua state to be used by engine call;
             * @return 1, the default output;
             */
            int LuaSandboxApi :: IoOutput( lua_State* pLuaState )  {

                return SetDefaultFile( pLuaState, __DEFAULT_OUTPUT_KEY, "w" );
            }

            /**
             * @brief io.read( ... ): io.input():read( ... ).
             *
             * @param pLuaState Lua state to be used by engine call;
             * @return What the read returns;
             */
            int LuaSandboxApi :: IoRead( lua_State* pLuaState )  {

                lua_getfield( pLuaState, LUA_REGISTRYINDEX, __DEFAULT_INPUT_KEY );
                lua_insert( pLuaState, 1 );

                return CallFileMethod( pLuaState, "read" );
            }

            /**
             * @brief io.write( ... ): io.output():write( ... ).
             *
             * @param pLuaState Lua state to be used by engine call;
             * @return What the write returns (the file, or nil and a message);
             */
            int LuaSandboxApi :: IoWrite( lua_State* pLuaState )  {

                lua_getfield( pLuaState, LUA_REGISTRYINDEX, __DEFAULT_OUTPUT_KEY );
                lua_insert( pLuaState, 1 );

                return CallFileMethod( pLuaState, "write" );
            }

            /**
             * @brief io.close( [file] ): file:close(), the default output's when no file is given.
             *
             * @param pLuaState Lua state to be used by engine call;
             * @return What the close returns;
             */
            int LuaSandboxApi :: IoClose( lua_State* pLuaState )  {

                if( lua_isnone( pLuaState, 1 ) )
                    lua_getfield( pLuaState, LUA_REGISTRYINDEX, __DEFAULT_OUTPUT_KEY );

                lua_settop( pLuaState, 1 );

                return CallFileMethod( pLuaState, "close" );
            }

            /**
             * @brief io.type( obj ): "file", "closed file" or nil - for the sandbox's files and Lua's own.
             *
             * @param pLuaState Lua state to be used by engine call;
             * @return 1;
             */
            int LuaSandboxApi :: IoType( lua_State* pLuaState )  {

                luaL_checkany( pLuaState, 1 );

                stLuaFile*  pFile = static_cast<stLuaFile *>( luaL_testudata( pLuaState, 1, __FILE_METATABLE ) );

                if( pFile )  {
                    lua_pushstring( pLuaState, pFile -> bClosed ? "closed file" : "file" );
                    return 1;
                }

                luaL_Stream*  pStream = static_cast<luaL_Stream *>( luaL_testudata( pLuaState, 1, LUA_FILEHANDLE ) );

                if( !pStream )
                    luaL_pushfail( pLuaState );
                else
                    lua_pushstring( pLuaState, pStream -> closef ? "file" : "closed file" );

                return 1;
            }

            /**
             * @brief io.popen( prog [, mode] ): refused - there is no process to run in the sandbox.
             *
             * @param pLuaState Lua state to be used by engine call;
             * @return nil, a message and a code;
             */
            int LuaSandboxApi :: IoPopen( lua_State* pLuaState )  {

                const char*  szProgram = luaL_checkstring( pLuaState, 1 );

                return PushFailure( pLuaState, szProgram, EACCES, "io.popen" __NOT_AVAILABLE );
            }

            /**
             * @brief io.tmpfile(): a file kept in memory ("w+"), never written anywhere.
             *
             * @param pLuaState Lua state to be used by engine call;
             * @return 1, the file;
             */
            int LuaSandboxApi :: IoTmpFile( lua_State* pLuaState )  {

                Engine :: SandboxFile :: stMode  mode;

                Engine :: SandboxFile :: ParseMode( "w+", mode );
                PushNewFile( pLuaState, mode, std :: vector<char>(), std :: string() );

                return 1;
            }

            /**
             * @brief os.remove( filename ): remove a file from the save directory (and only from there).
             *
             * @param pLuaState Lua state to be used by engine call;
             * @return true, or nil, a message and a code;
             */
            int LuaSandboxApi :: OsRemove( lua_State* pLuaState )  {

                const char*                   szName = luaL_checkstring( pLuaState, 1 );
                std :: string                 strSavePath;
                std :: string                 strError;
                std :: vector<unsigned char>  data;

                if( !ResolveWrite( pLuaState, szName, strSavePath, strError ) )
                    return PushFailure( pLuaState, szName, EACCES, strError );

                if( !ReadSaveFile( strSavePath, data ) )
                    return PushFailure( pLuaState, szName, ENOENT );

                if( !SunLight :: FileSystem :: FileSystemFactory :: GetFileSystem().RemoveFile( strSavePath ) )
                    return PushFailure( pLuaState, szName, EACCES );

                lua_pushboolean( pLuaState, 1 );

                return 1;
            }

            /**
             * @brief os.rename( oldname, newname ): rename a file inside the save directory (written to the new
             * name atomically, then the old one removed).
             *
             * @param pLuaState Lua state to be used by engine call;
             * @return true, or nil, a message and a code;
             */
            int LuaSandboxApi :: OsRename( lua_State* pLuaState )  {

                const char*                   szOldName = luaL_checkstring( pLuaState, 1 );
                const char*                   szNewName = luaL_checkstring( pLuaState, 2 );
                std :: string                 strOldPath;
                std :: string                 strNewPath;
                std :: string                 strError;
                std :: vector<unsigned char>  data;

                if( !ResolveWrite( pLuaState, szOldName, strOldPath, strError ) )
                    return PushFailure( pLuaState, szOldName, EACCES, strError );

                if( !ResolveWrite( pLuaState, szNewName, strNewPath, strError ) )
                    return PushFailure( pLuaState, szNewName, EACCES, strError );

                if( !ReadSaveFile( strOldPath, data ) )
                    return PushFailure( pLuaState, szOldName, ENOENT );

                if( strOldPath != strNewPath )  {
                    SunLight :: FileSystem :: IFileSystem&  fileSystem = SunLight :: FileSystem :: FileSystemFactory :: GetFileSystem();

                    if( !fileSystem.WriteFile( strNewPath, data ) || !fileSystem.RemoveFile( strOldPath ) )
                        return PushFailure( pLuaState, szOldName, EACCES );
                }

                lua_pushboolean( pLuaState, 1 );

                return 1;
            }

            /**
             * @brief Create a fresh, empty file in the save directory and push its name: os.tmpname's work, in a
             * frame of its own so the error is raised where no C++ object lives.
             *
             * @param pLuaState Lua state to be used by engine call;
             * @return true if the name was pushed, false when there is no save directory or no free name;
             */
            static bool PushNewTempName( lua_State* pLuaState )  {

                Engine :: SaveDirectory*  pSaveDirectory = GetSaveDirectory( pLuaState );

                if( !pSaveDirectory || pSaveDirectory -> GetPath().empty() )
                    return false;

                static std :: mt19937  random( std :: random_device {}() );

                const std :: string                             strLetters = __TMPNAME_LETTERS;
                std :: uniform_int_distribution<std :: size_t>  pick( 0, strLetters.size() - 1 );

                for( int nTry = 0; nTry < __TMPNAME_TRIES; nTry++ )  {
                    std :: string                 strName = __TMPNAME_PREFIX;
                    std :: vector<unsigned char>  data;

                    for( int nLetter = 0; nLetter < __TMPNAME_LENGTH; nLetter++ )
                        strName += strLetters[pick( random )];

                    if( ReadSaveFile( strName, data ) )
                        continue;           // taken: try another name

                    if( SunLight :: FileSystem :: FileSystemFactory :: GetFileSystem().WriteFile( strName, data ) )  {
                        lua_pushstring( pLuaState, strName.c_str() );
                        return true;
                    }
                }

                return false;
            }

            /**
             * @brief os.tmpname(): the name of a fresh, empty file created in the save directory (Lua's own
             * creates the file too) - a plain name, which io.open writes back to the same file. Raises, as Lua's
             * does, when it can't; this frame holds no C++ object for that longjmp (PushNewTempName does the
             * work - on Windows, unwinding through one crashed: C8, found in CI).
             *
             * @param pLuaState Lua state to be used by engine call;
             * @return 1, the name;
             */
            int LuaSandboxApi :: OsTmpName( lua_State* pLuaState )  {

                if( !PushNewTempName( pLuaState ) )
                    return luaL_error( pLuaState, "unable to generate a unique filename" );

                return 1;
            }

            /**
             * @brief os.execute( [command] ): no shell in the sandbox - false with no command, as Lua answers
             * "is there a shell?", and a refusal for a command.
             *
             * @param pLuaState Lua state to be used by engine call;
             * @return false, or nil, a message and a code;
             */
            int LuaSandboxApi :: OsExecute( lua_State* pLuaState )  {

                if( !luaL_optstring( pLuaState, 1, nullptr ) )  {
                    lua_pushboolean( pLuaState, 0 );
                    return 1;
                }

                return PushFailure( pLuaState, nullptr, EACCES, "os.execute" __NOT_AVAILABLE );
            }

            /**
             * @brief package.loadlib( path, funcname ): no C libraries in the sandbox - Lua's own answer when
             * dynamic libraries are absent.
             *
             * @param pLuaState Lua state to be used by engine call;
             * @return nil, a message and "absent";
             */
            int LuaSandboxApi :: PackageLoadLib( lua_State* pLuaState )  {

                luaL_checkstring( pLuaState, 1 );
                luaL_checkstring( pLuaState, 2 );
                luaL_pushfail( pLuaState );
                lua_pushliteral( pLuaState, "package.loadlib" __NOT_AVAILABLE );
                lua_pushliteral( pLuaState, "absent" );

                return 3;
            }

            /**
             * @brief package.searchpath( name, path [, sep [, rep]] ): Lua's own, through the file system (the
             * path rule); rep defaults to '/', a virtual path's separator on every OS.
             *
             * @param pLuaState Lua state to be used by engine call;
             * @return The file, or nil and the files tried;
             */
            int LuaSandboxApi :: PackageSearchPath( lua_State* pLuaState )  {

                const char*    szName         = luaL_checkstring( pLuaState, 1 );
                const char*    szPath         = luaL_checkstring( pLuaState, 2 );
                const char*    szSeparator    = luaL_optstring( pLuaState, 3, "." );
                const char*    szDirSeparator = luaL_optstring( pLuaState, 4, __VIRTUAL_SEPARATOR );
                std :: string  strFound;
                std :: string  strTried;

                if( FindInPath( szName, szPath, szSeparator, szDirSeparator, strFound, strTried ) )  {
                    lua_pushstring( pLuaState, strFound.c_str() );
                    return 1;
                }

                luaL_pushfail( pLuaState );
                lua_pushstring( pLuaState, strTried.c_str() );

                return 2;
            }

            /**
             * @brief Find and load a Lua module by package.path: SearchLuaModule's work, in a frame of its own so
             * the error is raised where no C++ object lives.
             *
             * @param pLuaState Lua state to be used by engine call;
             * @param szName The module's name;
             * @param szPath package.path;
             * @return 1 with the files tried pushed (not found), 2 with the loader and its file pushed, or -1 with
             * the error message pushed;
             */
            static int FindAndLoadModule( lua_State* pLuaState, const char* szName, const char* szPath )  {

                std :: string  strFound;
                std :: string  strTried;

                if( !FindInPath( szName, szPath, ".", __VIRTUAL_SEPARATOR, strFound, strTried ) )  {
                    lua_pushstring( pLuaState, strTried.c_str() );
                    return 1;
                }

                if( LuaFileSystemApi :: LoadChunk( pLuaState, strFound, __TEXT_MODE ) == LUA_OK )  {
                    lua_pushstring( pLuaState, strFound.c_str() );
                    return 2;
                }

                lua_pushfstring( pLuaState, "error loading module '%s' from file '%s':\n\t%s", szName,
                                 strFound.c_str(), lua_tostring( pLuaState, -1 ) );

                return -1;
            }

            /**
             * @brief require's Lua-module searcher (package.searchers[2]): the module's file by package.path,
             * read through the file system and loaded as text - Lua's searcher_Lua, by the path rule. This frame
             * holds no C++ object for the error's longjmp (FindAndLoadModule does the work).
             *
             * @param pLuaState Lua state to be used by engine call;
             * @return The loader and the file's name, or the files tried;
             */
            int LuaSandboxApi :: SearchLuaModule( lua_State* pLuaState )  {

                const char*  szName = luaL_checkstring( pLuaState, 1 );

                lua_getfield( pLuaState, lua_upvalueindex( 1 ), "path" );

                const char*  szPath = lua_tostring( pLuaState, -1 );

                if( !szPath )
                    return luaL_error( pLuaState, "'package.path' must be a string" );

                int  nResults = FindAndLoadModule( pLuaState, szName, szPath );

                if( nResults < 0 )
                    return lua_error( pLuaState );

                return nResults;
            }

            /**
             * @brief require's C-module searchers (package.searchers[3] and [4]): there are none in the
             * sandbox.
             *
             * @param pLuaState Lua state to be used by engine call;
             * @return 1, the searcher's explanation;
             */
            int LuaSandboxApi :: SearchCModule( lua_State* pLuaState )  {

                luaL_checkstring( pLuaState, 1 );
                lua_pushliteral( pLuaState, "no C modules in the sandbox" );

                return 1;
            }

            /**
             * @brief debug.setmetatable in the sandbox (plans/MASTER_PLAN.md item C8a): Lua's own (lbaselib's
             * db_setmetatable) for every value but a full userdata, whose metatable says what kind of engine object
             * it is - giving a Lua file the sandbox file's metatable, or the other way round, made one pass for the
             * other (shown: the process aborted when it was collected).
             *
             * @param pLuaState Lua state to be used by engine call;
             * @return The value, or nil and a message for a full userdata;
             */
            int LuaSandboxApi :: DebugSetMetatable( lua_State* pLuaState )  {

                int  nType = lua_type( pLuaState, 2 );

                luaL_argexpected( pLuaState, ( nType == LUA_TNIL ) || ( nType == LUA_TTABLE ), 2, "nil or table" );

                if( lua_type( pLuaState, 1 ) == LUA_TUSERDATA )  {
                    luaL_pushfail( pLuaState );
                    lua_pushliteral( pLuaState, "debug.setmetatable on a userdata" __NOT_AVAILABLE );
                    return 2;
                }

                lua_settop( pLuaState, 2 );
                lua_setmetatable( pLuaState, 1 );

                return 1;
            }

            /**
             * @brief debug.getregistry in the sandbox (plans/MASTER_PLAN.md item C8a): refused - the registry holds
             * the engine's metatables, and replacing one there makes one object pass for another, as
             * debug.setmetatable would.
             *
             * @param pLuaState Lua state to be used by engine call;
             * @return nil and a message;
             */
            int LuaSandboxApi :: DebugGetRegistry( lua_State* pLuaState )  {

                luaL_pushfail( pLuaState );
                lua_pushliteral( pLuaState, "debug.getregistry" __NOT_AVAILABLE );

                return 2;
            }

            /**
             * @brief loadfile in the sandbox: Lua's own, through the file system (the path rule), text chunks
             * only.
             *
             * @param pLuaState Lua state to be used by engine call;
             * @return The chunk as a function, or nil and a message;
             *
             * @luaname{loadfile(filename [, mode [, env]]) -> function | nil, message}
             * @luadoc
             * Lua's own `loadfile`, in the sandbox: the file is found by the path rule (the save
             * directory, then the game's own files) through the same file system as everything else
             * (`.zip` and encrypted packs included), and only text chunks load — a `mode` without `"t"`
             * gives `nil, "binary chunks are not available in the sandbox ..."`. Reading standard input
             * (no file name) is not available either. `--no-sandbox` gives back Lua's own.
             * @luaexample
             * local chunk, err = loadfile( "mods/extra_level.lua" )
             * if chunk then chunk() else print( err ) end
             */
            int LuaSandboxApi :: LoadFile( lua_State* pLuaState )  {

                const char*  szName     = luaL_optstring( pLuaState, 1, nullptr );
                const char*  szMode     = luaL_optstring( pLuaState, 2, __ANY_MODE );
                int          nEnvIndex  = lua_isnone( pLuaState, __LOADFILE_ENV_ARG ) ? 0 : __LOADFILE_ENV_ARG;

                if( !szName )  {
                    luaL_pushfail( pLuaState );
                    lua_pushliteral( pLuaState, "loadfile from standard input" __NOT_AVAILABLE );
                    return 2;
                }

                if( !std :: strchr( szMode, 't' ) )
                    return PushBinaryRefused( pLuaState, szMode );

                return LoadResult( pLuaState, LuaFileSystemApi :: LoadChunk( pLuaState, szName, __TEXT_MODE ), nEnvIndex );
            }

            /**
             * @brief load in the sandbox: Lua's own, text chunks only.
             *
             * @param pLuaState Lua state to be used by engine call;
             * @return The chunk as a function, or nil and a message;
             *
             * @luaname{load(chunk [, chunkname [, mode [, env]]]) -> function | nil, message}
             * @luadoc
             * Lua's own `load` (a string, or a function returning the pieces), in the sandbox: text
             * chunks only — precompiled bytecode can crash the interpreter, so a binary chunk gives
             * `nil, "attempt to load a binary chunk (mode is 't')"`, and a `mode` without `"t"` gives
             * `nil, "binary chunks are not available in the sandbox ..."`. `--no-sandbox` gives back
             * Lua's own.
             * @luaexample
             * local add = assert( load( "return function( a, b ) return a + b end" ) )()
             */
            int LuaSandboxApi :: Load( lua_State* pLuaState )  {

                std :: size_t  nSize      = 0;
                const char*    szChunk    = lua_tolstring( pLuaState, 1, &nSize );
                const char*    szMode     = luaL_optstring( pLuaState, 3, __ANY_MODE );
                int            nEnvIndex  = lua_isnone( pLuaState, __LOAD_ENV_ARG ) ? 0 : __LOAD_ENV_ARG;
                int            nStatus;

                if( !std :: strchr( szMode, 't' ) )
                    return PushBinaryRefused( pLuaState, szMode );

                if( szChunk )  {
                    const char*  szChunkName = luaL_optstring( pLuaState, 2, szChunk );

                    nStatus = luaL_loadbufferx( pLuaState, szChunk, nSize, szChunkName, __TEXT_MODE );
                }
                else  {
                    const char*  szChunkName = luaL_optstring( pLuaState, 2, "=(load)" );

                    luaL_checktype( pLuaState, 1, LUA_TFUNCTION );
                    lua_settop( pLuaState, __CHUNK_RESERVED_SLOT );
                    nStatus = lua_load( pLuaState, ReadChunkPiece, nullptr, szChunkName, __TEXT_MODE );
                }

                return LoadResult( pLuaState, nStatus, nEnvIndex );
            }

            /**
             * @brief file:close(): write the file back if it changed, then close it.
             *
             * @param pLuaState Lua state to be used by engine call;
             * @return true, or nil, a message and a code when it could not be written;
             */
            int LuaSandboxApi :: FileClose( lua_State* pLuaState )  {

                stLuaFile*     pFile = CheckOpenFile( pLuaState, 1 );
                std :: string  strError;
                bool           bWritten = WriteBack( pFile, strError );

                pFile -> bClosed = true;

                if( !bWritten )
                    return PushFailure( pLuaState, nullptr, EIO, strError );

                lua_pushboolean( pLuaState, 1 );

                return 1;
            }

            /**
             * @brief file:flush(): write the file back now if it changed.
             *
             * @param pLuaState Lua state to be used by engine call;
             * @return true, or nil, a message and a code;
             */
            int LuaSandboxApi :: FileFlush( lua_State* pLuaState )  {

                stLuaFile*     pFile = CheckOpenFile( pLuaState, 1 );
                std :: string  strError;

                if( !WriteBack( pFile, strError ) )
                    return PushFailure( pLuaState, nullptr, EIO, strError );

                lua_pushboolean( pLuaState, 1 );

                return 1;
            }

            /**
             * @brief file:lines( ... ): an iterator reading by the formats ("l" by default), leaving the file
             * open.
             *
             * @param pLuaState Lua state to be used by engine call;
             * @return 1, the iterator;
             */
            int LuaSandboxApi :: FileLines( lua_State* pLuaState )  {

                CheckOpenFile( pLuaState, 1 );
                PushLinesIterator( pLuaState, false, LuaSandboxApi :: FileReadLine );

                return 1;
            }

            /**
             * @brief file:read( ... ): read by the formats ("l" by default).
             *
             * @param pLuaState Lua state to be used by engine call;
             * @return The values read;
             */
            int LuaSandboxApi :: FileRead( lua_State* pLuaState )  {

                return ReadFormats( pLuaState, CheckOpenFile( pLuaState, 1 ), 2 );
            }

            /**
             * @brief file:seek( [whence [, offset]] ): move the position, "cur" and 0 by default.
             *
             * @param pLuaState Lua state to be used by engine call;
             * @return The new position, or nil, a message and a code;
             */
            int LuaSandboxApi :: FileSeek( lua_State* pLuaState )  {

                static const char* const                           szWhenceNames[] = { "set", "cur", "end", nullptr };
                static const Engine :: SandboxFile :: SeekWhence  whences[]        = { Engine :: SandboxFile :: SEEK_WHENCE_SET,
                                                                                       Engine :: SandboxFile :: SEEK_WHENCE_CUR,
                                                                                       Engine :: SandboxFile :: SEEK_WHENCE_END };

                stLuaFile*     pFile     = CheckOpenFile( pLuaState, 1 );
                int            nWhence   = luaL_checkoption( pLuaState, 2, "cur", szWhenceNames );
                lua_Integer    nOffset   = luaL_optinteger( pLuaState, 3, 0 );
                std :: size_t  nPosition = 0;

                if( !pFile -> file.Seek( whences[nWhence], nOffset, nPosition ) )
                    return PushFailure( pLuaState, nullptr, EINVAL );

                lua_pushinteger( pLuaState, ( lua_Integer ) nPosition );

                return 1;
            }

            /**
             * @brief file:setvbuf( mode [, size] ): accepted, as Lua's (a sandbox file is written back whole,
             * so there is no buffer to set).
             *
             * @param pLuaState Lua state to be used by engine call;
             * @return 1, true;
             */
            int LuaSandboxApi :: FileSetVBuf( lua_State* pLuaState )  {

                static const char* const  szModeNames[] = { "no", "full", "line", nullptr };

                CheckOpenFile( pLuaState, 1 );
                luaL_checkoption( pLuaState, 2, nullptr, szModeNames );
                luaL_optinteger( pLuaState, 3, LUAL_BUFFERSIZE );
                lua_pushboolean( pLuaState, 1 );

                return 1;
            }

            /**
             * @brief file:write( ... ): write strings and numbers (numbers as Lua writes them: LUA_INTEGER_FMT
             * and LUA_NUMBER_FMT).
             *
             * @param pLuaState Lua state to be used by engine call;
             * @return The file, or nil, a message and a code;
             */
            int LuaSandboxApi :: FileWrite( lua_State* pLuaState )  {

                stLuaFile*  pFile = CheckOpenFile( pLuaState, 1 );
                int         nTop  = lua_gettop( pLuaState );

                if( !pFile -> file.CanWrite() )
                    return PushFailure( pLuaState, nullptr, EBADF );

                for( int nArg = 2; nArg <= nTop; nArg++ )  {
                    if( lua_type( pLuaState, nArg ) == LUA_TNUMBER )  {
                        char  szNumber[__NUMBER_BUFFER_SIZE];
                        int   nLength = lua_isinteger( pLuaState, nArg ) ?
                                        std :: snprintf( szNumber, sizeof( szNumber ), LUA_INTEGER_FMT, ( LUAI_UACINT ) lua_tointeger( pLuaState, nArg ) ) :
                                        std :: snprintf( szNumber, sizeof( szNumber ), LUA_NUMBER_FMT, ( LUAI_UACNUMBER ) lua_tonumber( pLuaState, nArg ) );

                        pFile -> file.Write( szNumber, ( std :: size_t ) nLength );
                        continue;
                    }

                    std :: size_t  nLength = 0;
                    const char*    szText  = luaL_checklstring( pLuaState, nArg, &nLength );

                    pFile -> file.Write( szText, nLength );
                }

                lua_pushvalue( pLuaState, 1 );

                return 1;
            }

            /**
             * @brief The file's __gc: write it back if it changed (and it was not closed), then destroy it.
             *
             * @param pLuaState Lua state to be used by engine call;
             * @return 0;
             */
            int LuaSandboxApi :: FileGc( lua_State* pLuaState )  {

                stLuaFile*  pFile = static_cast<stLuaFile *>( luaL_checkudata( pLuaState, 1, __FILE_METATABLE ) );

                Finish( pFile );
                pFile -> ~stLuaFile();

                return 0;
            }

            /**
             * @brief The file's __close (a to-be-closed variable going out of scope, as io.lines' fourth value
             * is): write it back if it changed and close it - the object itself lives until __gc.
             *
             * @param pLuaState Lua state to be used by engine call;
             * @return 0;
             */
            int LuaSandboxApi :: FileToBeClosed( lua_State* pLuaState )  {

                Finish( static_cast<stLuaFile *>( luaL_checkudata( pLuaState, 1, __FILE_METATABLE ) ) );

                return 0;
            }

            /**
             * @brief The file's __tostring: "file (closed)" or "file (0x...)", as Lua's own.
             *
             * @param pLuaState Lua state to be used by engine call;
             * @return 1;
             */
            int LuaSandboxApi :: FileToString( lua_State* pLuaState )  {

                stLuaFile*  pFile = static_cast<stLuaFile *>( luaL_checkudata( pLuaState, 1, __FILE_METATABLE ) );

                if( pFile -> bClosed )
                    lua_pushliteral( pLuaState, "file (closed)" );
                else
                    lua_pushfstring( pLuaState, "file (%p)", static_cast<void *>( pFile ) );

                return 1;
            }

            /**
             * @brief The lines iterator's body (liolib.c's io_readline): read by its formats; at the end of the
             * file, close the file if the iterator opened it; a read error is raised.
             *
             * @param pLuaState Lua state to be used by engine call;
             * @return The values read, none at the end;
             */
            int LuaSandboxApi :: FileReadLine( lua_State* pLuaState )  {

                // Checked, not trusted: debug.setupvalue can put anything in a C closure's upvalue (C8a).
                stLuaFile*  pFile    = static_cast<stLuaFile *>( luaL_testudata( pLuaState, lua_upvalueindex( 1 ), __FILE_METATABLE ) );
                int         nFormats = ( int ) lua_tointeger( pLuaState, lua_upvalueindex( 2 ) );

                if( !pFile )
                    return luaL_error( pLuaState, "the lines iterator's file was replaced" );

                if( ( nFormats < 0 ) || ( nFormats > __MAX_LINES_FORMATS ) )
                    return luaL_error( pLuaState, "the lines iterator's formats were replaced" );

                if( pFile -> bClosed )
                    return luaL_error( pLuaState, "file is already closed" );

                lua_settop( pLuaState, 1 );
                luaL_checkstack( pLuaState, nFormats, "too many arguments" );

                for( int nFormat = 1; nFormat <= nFormats; nFormat++ )
                    lua_pushvalue( pLuaState, lua_upvalueindex( 3 + nFormat ) );

                int  nResults = ReadFormats( pLuaState, pFile, 2 );

                if( lua_toboolean( pLuaState, -nResults ) )
                    return nResults;

                if( nResults > 1 )
                    return luaL_error( pLuaState, "%s", lua_tostring( pLuaState, -nResults + 1 ) );

                if( lua_toboolean( pLuaState, lua_upvalueindex( 3 ) ) )
                    Finish( pFile );

                return 0;
            }

            /**
             * @brief Create the sandbox files' metatable: Lua's file methods, and __gc, __close, __tostring.
             *
             * @param pLuaState Lua state to be used by engine call;
             */
            void LuaSandboxApi :: InstallFileType( lua_State* pLuaState )  {

                static const luaL_Reg  methods[] = {
                    { "close",   FileClose   },
                    { "flush",   FileFlush   },
                    { "lines",   FileLines   },
                    { "read",    FileRead    },
                    { "seek",    FileSeek    },
                    { "setvbuf", FileSetVBuf },
                    { "write",   FileWrite   },
                    { nullptr,   nullptr     }
                };
                static const luaL_Reg  metamethods[] = {
                    { "__gc",       FileGc         },
                    { "__close",    FileToBeClosed },
                    { "__tostring", FileToString   },
                    { nullptr,      nullptr        }
                };

                luaL_newmetatable( pLuaState, __FILE_METATABLE );
                luaL_setfuncs( pLuaState, metamethods, 0 );
                luaL_newlib( pLuaState, methods );
                lua_setfield( pLuaState, -2, "__index" );
                lua_pop( pLuaState, 1 );
            }

            /**
             * @brief Replace the io library's functions in place; the defaults start as io.stdin and io.stdout.
             *
             * @param pLuaState Lua state to be used by engine call;
             */
            void LuaSandboxApi :: InstallIo( lua_State* pLuaState )  {

                static const luaL_Reg  functions[] = {
                    { "open",    IoOpen    },
                    { "lines",   IoLines   },
                    { "input",   IoInput   },
                    { "output",  IoOutput  },
                    { "read",    IoRead    },
                    { "write",   IoWrite   },
                    { "close",   IoClose   },
                    { "type",    IoType    },
                    { "popen",   IoPopen   },
                    { "tmpfile", IoTmpFile },
                    { nullptr,   nullptr   }
                };

                lua_getglobal( pLuaState, LUA_IOLIBNAME );
                luaL_setfuncs( pLuaState, functions, 0 );
                lua_getfield( pLuaState, -1, "stdin" );
                lua_setfield( pLuaState, LUA_REGISTRYINDEX, __DEFAULT_INPUT_KEY );
                lua_getfield( pLuaState, -1, "stdout" );
                lua_setfield( pLuaState, LUA_REGISTRYINDEX, __DEFAULT_OUTPUT_KEY );
                lua_pop( pLuaState, 1 );
            }

            /**
             * @brief Replace the os library's file and process functions in place (the rest is kept).
             *
             * @param pLuaState Lua state to be used by engine call;
             */
            void LuaSandboxApi :: InstallOs( lua_State* pLuaState )  {

                static const luaL_Reg  functions[] = {
                    { "remove",  OsRemove  },
                    { "rename",  OsRename  },
                    { "tmpname", OsTmpName },
                    { "execute", OsExecute },
                    { nullptr,   nullptr   }
                };

                lua_getglobal( pLuaState, LUA_OSLIBNAME );
                luaL_setfuncs( pLuaState, functions, 0 );
                lua_pop( pLuaState, 1 );
            }

            /**
             * @brief Point require at the file system: package.path by the path rule, no cpath, Lua's own Lua
             * searcher replaced, the C ones refusing, and loadlib/searchpath replaced. require itself and the
             * preload searcher are Lua's own.
             *
             * @param pLuaState Lua state to be used by engine call;
             */
            void LuaSandboxApi :: InstallPackage( lua_State* pLuaState )  {

                lua_getglobal( pLuaState, LUA_LOADLIBNAME );
                lua_pushstring( pLuaState, PACKAGE_PATH );
                lua_setfield( pLuaState, -2, "path" );
                lua_pushliteral( pLuaState, "" );
                lua_setfield( pLuaState, -2, "cpath" );
                lua_pushcfunction( pLuaState, PackageLoadLib );
                lua_setfield( pLuaState, -2, "loadlib" );
                lua_pushcfunction( pLuaState, PackageSearchPath );
                lua_setfield( pLuaState, -2, "searchpath" );
                lua_getfield( pLuaState, -1, "searchers" );
                lua_pushvalue( pLuaState, -2 );
                lua_pushcclosure( pLuaState, SearchLuaModule, 1 );
                lua_rawseti( pLuaState, -2, 2 );
                lua_pushcfunction( pLuaState, SearchCModule );
                lua_rawseti( pLuaState, -2, 3 );
                lua_pushcfunction( pLuaState, SearchCModule );
                lua_rawseti( pLuaState, -2, 4 );
                lua_pop( pLuaState, 2 );
            }

            /**
             * @brief Replace the two debug functions that let a script give an engine object another type's
             * metatable (C8a); the rest of the debug library is Lua's own.
             *
             * @param pLuaState Lua state to be used by engine call;
             */
            void LuaSandboxApi :: InstallDebug( lua_State* pLuaState )  {

                static const luaL_Reg  functions[] = {
                    { "setmetatable", DebugSetMetatable },
                    { "getregistry",  DebugGetRegistry  },
                    { nullptr,        nullptr           }
                };

                lua_getglobal( pLuaState, LUA_DBLIBNAME );
                luaL_setfuncs( pLuaState, functions, 0 );
                lua_pop( pLuaState, 1 );
            }

            /**
             * @brief Install the sandbox: Lua's io, os, package, loadfile, load, debug.setmetatable and debug.getregistry replaced in place. Must run
             * right after luaL_openlibs, before any script.
             *
             * @param pLuaState Lua state to be used by engine call;
             * @param strAppDirectory APP_DIR (a path starting with it is written under the part after it);
             */
            void LuaSandboxApi :: Install( lua_State* pLuaState, const std :: string& strAppDirectory )  {

                lua_pushboolean( pLuaState, 1 );
                lua_setfield( pLuaState, LUA_REGISTRYINDEX, __INSTALLED_KEY );
                lua_pushstring( pLuaState, strAppDirectory.c_str() );
                lua_setfield( pLuaState, LUA_REGISTRYINDEX, __APP_DIRECTORY_KEY );

                InstallFileType( pLuaState );
                InstallIo( pLuaState );
                InstallOs( pLuaState );
                InstallPackage( pLuaState );
                InstallDebug( pLuaState );

                lua_register( pLuaState, "loadfile", LuaSandboxApi :: LoadFile );
                lua_register( pLuaState, "load", LuaSandboxApi :: Load );
            }

            /**
             * @brief Whether the sandbox is installed in this Lua state.
             *
             * @param pLuaState Lua state to be used by engine call;
             * @return true if it is;
             */
            bool LuaSandboxApi :: IsInstalled( lua_State* pLuaState )  {

                lua_getfield( pLuaState, LUA_REGISTRYINDEX, __INSTALLED_KEY );

                bool  bInstalled = lua_toboolean( pLuaState, -1 );

                lua_pop( pLuaState, 1 );

                return bInstalled;
            }

            /**
             * @brief The chunk mode code loads with: text only in the sandbox, Lua's "bt" without it.
             *
             * @param pLuaState Lua state to be used by engine call;
             * @return "t" or "bt";
             */
            const char* LuaSandboxApi :: GetChunkMode( lua_State* pLuaState )  {

                return IsInstalled( pLuaState ) ? __TEXT_MODE : __ANY_MODE;
            }

            /**
             * @brief Where a path is written, inside the save directory (plans/MASTER_PLAN.md item C8): the path
             * relative to it, with a leading APP_DIR taken off (decision 1b: APP_DIR .. "x" reads the save's "x"
             * first, so that is where it is written), "vpath://" and leading '/'s dropped, and "." and ".."
             * collapsed. A host path, a path with an unknown scheme, one that climbs out, and one that names no
             * file are refused. Pure string logic (unit-tested).
             *
             * @param strPath The path a script gave;
             * @param strAppDirectory APP_DIR, ending in '/' ("" for none);
             * @param strSavePath Receives the path inside the save directory;
             * @param strError Receives the reason on failure;
             * @return true if the path can be written;
             */
            bool LuaSandboxApi :: ToSavePath( const std :: string& strPath, const std :: string& strAppDirectory,
                                              std :: string& strSavePath, std :: string& strError )  {

                std :: string  strRest;

                switch( SunLight :: FileSystem :: FilePath :: SplitPath( strPath, strRest ) )  {
                    case SunLight :: FileSystem :: FilePath :: PATH_SCHEME_VIRTUAL :
                        break;
                    case SunLight :: FileSystem :: FilePath :: PATH_SCHEME_HOST :
                        strError = "a host path can't be written in the sandbox - only the save directory";
                        return false;
                    default :
                        strError = "unknown path scheme";
                        return false;
                }

                if( !strAppDirectory.empty() && ( strRest.compare( 0, strAppDirectory.size(), strAppDirectory ) == 0 ) )
                    strRest = strRest.substr( strAppDirectory.size() );

                if( strRest.empty() || ( strRest.back() == '/' ) || ( strRest.back() == '\\' ) )  {
                    strError = "not a file name";
                    return false;
                }

                std :: string  strJoined = SunLight :: FileSystem :: FilePath :: JoinPath( "", strRest );
                std :: size_t  nStart    = strJoined.find_first_not_of( '/' );

                if( nStart == std :: string :: npos )  {
                    strError = "climbs out of the save directory";
                    return false;
                }

                strSavePath = strJoined.substr( nStart );

                return true;
            }
        }
    }
}
