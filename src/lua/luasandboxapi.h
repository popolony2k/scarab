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

#ifndef __LUASANDBOXAPI_H__
#define __LUASANDBOXAPI_H__

#include <string>

extern "C"  {
  #include "lua.h"
}


namespace Scarab  {
    namespace Engine  {
        namespace Lua  {

            /**
             * @brief The sandbox (plans/MASTER_PLAN.md item C8): Lua's own file, process and code-loading
             * functions replaced IN PLACE, in C++, by ones that reach only the game's own files and its save
             * directory. Installed by LuaEngine right after luaL_openlibs, unless the command line says
             * --no-sandbox (or the engine runs the --pack tool); the originals are kept nowhere, so nothing -
             * the debug library included - can get them back.
             *
             * The path rule itself is the file system's mounts (main.cpp, EngineHost::ResolveEntryScript,
             * SaveDirectory): reading is a plain FileSystem read, writing goes into the save directory
             * (ToSavePath), and host paths are refused by sunlight (SetHostPathEnabled( false )).
             *
             * @luacategory{The sandbox and how paths are found}
             * @luadoc
             * Every path a game's script uses — in `io.open`, `dofile`, `require`, `loadfile`,
             * `load_json`, maps (with their tilesets and images), textures, fonts, sounds and the action
             * map — is found by **one rule**, in every mode:
             *
             * - **Reading** a path looks in **(1) the save directory**, then **(2) the game's own
             *   files**: its `.zip` when the game was launched from one, otherwise the launch folder.
             * - **Writing** a path always goes into **the save directory** (there must be one: see
             *   `app_set_save_directory`, the project file's `"save_directory"` or `--save-dir`).
             * - A path that starts with **`APP_DIR`** follows the same rule: the save directory first,
             *   under the part after `APP_DIR` (`APP_DIR .. "src/ai.lua"` → the save's `src/ai.lua`),
             *   then exactly where `APP_DIR` points — the executable's folder, or the game's `.zip`.
             *   Plain relative paths (`"resources/hero.png"`) are the recommended form.
             *
             * So a saved file takes the place of the game's own file of the same name — that is how mods
             * and patches work, code included (`dofile`, `require`): a crafted save can run code, but only
             * inside the sandbox. The `save_*` functions are different: they read and write saves only,
             * never a game file.
             *
             * **The sandbox** is on by default: a script can reach nothing outside the game's own files
             * and its save directory. Lua's own functions keep working the way a Lua developer knows them,
             * replaced in place by Scarab (in C++) with ones that follow the rule; every failure is Lua's
             * own `nil, message`. Only the command line turns it off (`scarab --no-sandbox ...`) — a
             * project file cannot; the `--pack` tool runs without it. `hpath://` paths are refused in the
             * sandbox, and the `pack_*` functions exist only in the `--pack` tool.
             * @luaoutro
             * ## Lua's own functions in the sandbox
             *
             * | Function | In the sandbox |
             * |---|---|
             * | `io.open( name, "r" / "rb" )` | reads by the rule: the save directory, then the game's files |
             * | `io.open( name, "w" / "a" / "r+" / "w+" / "a+" )`, with or without `"b"` | writes into the save directory, the whole file at once (atomically) on `close`, `flush` or when the file is collected; `"a"`/`"r+"`/`"a+"` start from the saved file, or the game's own when there is no save |
             * | `io.lines( name )`, `io.input( name )`, `io.output( name )` | through `io.open`, the same rule |
             * | `io.read`, `io.write`, `io.close`, `io.lines()`, `io.type` | the default input/output, Lua's own files or the sandbox's |
             * | `io.tmpfile()` | a file kept in memory, never on disk |
             * | `io.popen`, `os.execute` | `nil, "... not available in the sandbox"` (`os.execute()` with no command: `false`, no shell) |
             * | `os.remove`, `os.rename` | inside the save directory only |
             * | `os.tmpname()` | a fresh, empty file in the save directory, by its name |
             * | `require` | Lua modules found by `package.path` (`"?.lua;?/init.lua"`) through the rule; C modules and `package.loadlib` fail the standard way |
             * | `loadfile`, `dofile` | by the rule; text chunks only |
             * | `load` | text chunks only (precompiled bytecode can crash the interpreter) |
             * | `debug.setmetatable` | as Lua's for every value but a full userdata (a file, ...), which gives `nil, message`: another type's metatable would make the engine take one object for another |
             * | `debug.getregistry` | `nil, message`: the registry holds the engine's metatables, swapping one has the same effect |
             * | `io.stdout`, `io.stderr`, `io.stdin`, `os.time`, `os.clock`, `os.date`, `os.difftime`, `os.getenv`, `os.exit`, the rest of `debug` | unchanged |
             *
             * A sandbox file has every method of a Lua file: `read( "a" / "l" / "L" / "n" / count )`,
             * `lines`, `write`, `seek`, `flush`, `setvbuf`, `close`, and `io.type` answers `"file"` or
             * `"closed file"` for it.
             *
             * A sandbox file is read and written **byte for byte on every OS** - as if every mode had
             * `"b"`: Lua's own `"r"` on Windows turns `"\r\n"` into `"\n"`, the sandbox never does, so a
             * game reads the same bytes everywhere, from a folder, a `.zip` or a save. A file saved with
             * Windows line endings keeps its `"\r"` at the end of each `"l"` line.
             *
             * ```lua
             * -- the game's own file, or the player's saved copy of it
             * local f = assert( io.open( "settings.txt" ) )
             * local text = f:read( "a" )
             * f:close()
             *
             * -- always into the save directory
             * local out = assert( io.open( "settings.txt", "w" ) )
             * out:write( text, "\nvolume=8\n" )
             * assert( out:close() )
             * ```
             */
            class LuaSandboxApi  {

                static int IoOpen( lua_State* pLuaState );
                static int IoLines( lua_State* pLuaState );
                static int IoInput( lua_State* pLuaState );
                static int IoOutput( lua_State* pLuaState );
                static int IoRead( lua_State* pLuaState );
                static int IoWrite( lua_State* pLuaState );
                static int IoClose( lua_State* pLuaState );
                static int IoType( lua_State* pLuaState );
                static int IoPopen( lua_State* pLuaState );
                static int IoTmpFile( lua_State* pLuaState );
                static int OsRemove( lua_State* pLuaState );
                static int OsRename( lua_State* pLuaState );
                static int OsTmpName( lua_State* pLuaState );
                static int OsExecute( lua_State* pLuaState );
                static int PackageLoadLib( lua_State* pLuaState );
                static int PackageSearchPath( lua_State* pLuaState );
                static int SearchLuaModule( lua_State* pLuaState );
                static int SearchCModule( lua_State* pLuaState );
                static int DebugSetMetatable( lua_State* pLuaState );
                static int DebugGetRegistry( lua_State* pLuaState );
                static int LoadFile( lua_State* pLuaState );
                static int Load( lua_State* pLuaState );

                static int FileClose( lua_State* pLuaState );
                static int FileFlush( lua_State* pLuaState );
                static int FileLines( lua_State* pLuaState );
                static int FileRead( lua_State* pLuaState );
                static int FileSeek( lua_State* pLuaState );
                static int FileSetVBuf( lua_State* pLuaState );
                static int FileWrite( lua_State* pLuaState );
                static int FileGc( lua_State* pLuaState );
                static int FileToBeClosed( lua_State* pLuaState );
                static int FileToString( lua_State* pLuaState );
                static int FileReadLine( lua_State* pLuaState );

                static void InstallFileType( lua_State* pLuaState );
                static void InstallIo( lua_State* pLuaState );
                static void InstallOs( lua_State* pLuaState );
                static void InstallPackage( lua_State* pLuaState );
                static void InstallDebug( lua_State* pLuaState );

                public:

                /** @brief The package.path the sandbox starts with: Lua modules by the path rule. */
                static constexpr const char*  PACKAGE_PATH = "?.lua;?/init.lua";

                static void Install( lua_State* pLuaState, const std :: string& strAppDirectory );
                static bool IsInstalled( lua_State* pLuaState );
                static const char* GetChunkMode( lua_State* pLuaState );
                static bool ToSavePath( const std :: string& strPath, const std :: string& strAppDirectory,
                                        std :: string& strSavePath, std :: string& strError );
            };
        }
    }
}

#endif /* __LUASANDBOXAPI_H__ */
