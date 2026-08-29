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

#ifndef __LUAFILESYSTEMAPI_H__
#define __LUAFILESYSTEMAPI_H__

extern "C"
{
  #include "lua.h"
}


namespace Scarab  {
    namespace Engine  {
        namespace Lua  {
            /**
             * @brief Phase 12 prototype (bundled/archived distribution,
             * game-engine repo) - overrides Lua's own built-in dofile()
             * with a version that reads through
             * SunLight::FileSystem::FileSystemFactory instead of it's
             * standard fopen()-based one, matching every other subsystem
             * already routed the same way (textures/sound/tilemaps/
             * load_json). This is the last piece: every resource this
             * engine ever reads now goes through one mount, whether that
             * mount is a real loose directory (today) or a real archive
             * (later) - nothing above this needed to change to notice the
             * difference, load_json/dofile call sites included.
             *
             * require()/loadfile()/raw io.* file access are deliberately
             * NOT overridden - confirmed, project-wide, that no script in
             * this game ever calls any of them; dofile() is the only
             * module-loading mechanism actually in use.
             */
            class LuaFileSystemApi  {

                static int DoFile( lua_State *pLuaState );

                public:

                static void Register( lua_State *pLuaState );
            };
        }
    }
}

#endif  /* __LUAFILESYSTEMAPI_H__ */
