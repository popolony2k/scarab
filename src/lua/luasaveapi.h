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

#ifndef __LUASAVEAPI_H__
#define __LUASAVEAPI_H__

#include <string>

extern "C"  {
  #include "lua.h"
}


namespace Scarab  {
    namespace Engine  {
        namespace Lua  {
            /**
             * @brief The game's save files (plans/MASTER_PLAN.md item C1c): Lua tables kept as JSON in the
             * save directory (Engine::SaveDirectory, C1b), written atomically by sunlight's
             * IFileSystem::WriteFile and removed by IFileSystem::RemoveFile.
             *
             * @luacategory{Save}
             * @luadoc
             * Keep a game's own data - options, high scores, progress - between runs. A save is a Lua
             * table written as JSON into the game's **save directory** (name it with `"save_directory"`
             * in `project.json` or with `app_set_save_directory`; `--save-dir <folder>` on the command
             * line puts it somewhere else). Every function returns `nil, "message"` on failure.
             *
             * - **Safe writes:** a save is written to a temporary file, synced to the disk and then
             *   renamed over the old one, so a crash or a power cut never leaves a half-written save -
             *   the old one is kept instead.
             * - **Only the save directory:** a save's name is a relative path (`"options.json"`,
             *   `"slots/1.json"`) made of letters, digits, `-`, `_` and `.`, with no `..`; it can never
             *   reach outside the save directory, and `save_read` never finds a file shipped with the
             *   game, even one of the same name.
             * - **The same table back:** integers stay integers, floats stay floats, strings and
             *   booleans as they were. A table whose keys are `1..n` is a JSON array; one with string
             *   keys a JSON object; an empty table `{}`. What JSON can't hold exactly is refused, with
             *   where in the table: mixed or other kinds of keys, holes, functions, a table that
             *   contains itself, NaN and infinity, text that is not UTF-8.
             * - **Plain JSON:** saves are never encrypted, even in an encrypted game, and keys are
             *   written sorted - the same table always gives the same file.
             *
             * ```lua
             * local options = save_read( "options.json" ) or { volume = 8, fullscreen = false }
             *
             * options.volume = options.volume + 1
             * assert( save_write( "options.json", options ) )
             * ```
             */
            class LuaSaveApi  {

                static int Write( lua_State* pLuaState );
                static int Read( lua_State* pLuaState );
                static int Exists( lua_State* pLuaState );
                static int Delete( lua_State* pLuaState );

                public:

                static bool IsValidSaveName( const std :: string& strName, std :: string& strError );
                static void Register( lua_State* pLuaState );
            };
        }
    }
}

#endif /* __LUASAVEAPI_H__ */
