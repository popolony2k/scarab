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

#ifndef __LUAPACKAPI_H__
#define __LUAPACKAPI_H__

extern "C"
{
  #include "lua.h"
}


namespace Scarab  {
    namespace Engine  {
        namespace Lua  {
            /**
             * @brief Packaging-tool support for content encryption -
             * see the root README.md's own "Content encryption" section
             * and tools/README.md. Backs
             * `tools/pack.lua`, the Lua script that actually walks a
             * game's source tree and produces an encrypted .zip bundle
             * (calling `crypto_encrypt_data` - see LuaCryptoApi - on
             * each file's bytes before adding it here).
             *
             * @luacategory{Pack}
             * @luadoc
             * Every primitive here reads/writes a genuine native OS
             * path, resolved exactly the way a plain C `fopen()` would
             * - **not** routed through the mount-based virtual
             * filesystem (`SunLight::FileSystem`) every other resource
             * load in this engine uses. That's deliberate: packaging
             * runs against a real, not-yet-bundled source directory on
             * disk, before any of Scarab's own runtime mounting
             * concepts (`APP_DIR`, a loaded `.zip`) are even relevant.
             * These primitives exist purely for `tools/pack.lua` - no
             * running game should ever call them.
             * @luaoutro
             * ## Archive handles
             *
             * `pack_create_archive`/`pack_add_entry`/`pack_close_archive`
             * share one integer-handle convention: `0` always means
             * failure (the same "0 is never a valid handle" shape
             * `sprite_acquire` already uses), a real handle otherwise.
             * Always call `pack_close_archive` once done with a handle
             * - the underlying zip file isn't actually finalized/valid
             * until then.
             */
            class LuaPackApi  {

                static int ListDirectory( lua_State *pLuaState );
                static int ReadFile( lua_State *pLuaState );
                static int WriteFile( lua_State *pLuaState );
                static int CreateArchive( lua_State *pLuaState );
                static int AddEntry( lua_State *pLuaState );
                static int CloseArchive( lua_State *pLuaState );

                public:

                static void Register( lua_State *pLuaState );
            };
        }
    }
}

#endif  /* __LUAPACKAPI_H__ */
