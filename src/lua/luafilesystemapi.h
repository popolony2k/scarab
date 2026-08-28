/*
 * luafilesystemapi.h
 *
 *  Created on: Aug 27, 2026
 *      Author: popolony2k
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
