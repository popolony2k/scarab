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

#ifndef __SAVEDIRECTORY_H__
#define __SAVEDIRECTORY_H__

#include <functional>
#include <string>


namespace Scarab  {
    namespace Engine  {

        /**
         * @brief The game's save directory (plans/MASTER_PLAN.md item C1b): the real folder sunlight writes
         * a virtual path into (IFileSystem::SetWriteDirectory), mounted ahead of every mount so a save reads
         * back by the same virtual path and overrides a file of that name shipped in the game.
         *
         * A game names its folder - in its project file ("save_directory") and/or with
         * app_set_save_directory - and Scarab puts it in the OS's per-user data folder (UserDataFolder). The
         * command line's --save-dir names the real folder itself and wins over both (tests, CI, portable
         * installs); app_set_save_directory wins over the project file, because it is applied later.
         */
        class SaveDirectory  {

            std :: string  m_strPath;                    // the real folder in use, "" while there is none
            bool           m_bFromCommandLine;           // --save-dir set it: a name no longer changes it
            std :: string  m_strGameMountPoint;          // APP_DIR: the save folder is mounted there too (C8)
            std :: string  m_strMountedPath;             // the folder the extra mounts are of, "" when none
            std :: string  m_strMountedGameMountPoint;   // where it is mounted besides SAVE_MOUNT_POINT

            bool Apply( const std :: string& strRealPath, std :: string& strError );
            bool MountExtras( void );
            void UnmountExtras( void );

            public:

            /** @brief The private mount point of the save folder alone, which the save functions read through. */
            static constexpr const char*  SAVE_MOUNT_POINT = "/__scarab_saves__";

            /** @brief Reads an environment variable: nullptr when it is not set. */
            typedef std :: function<const char*( const char* )>  EnvLookup;

            SaveDirectory( void );

            void SetGameMountPoint( const std :: string& strMountPoint );
            bool Remount( std :: string& strError );
            static std :: string GetSavePath( const std :: string& strName );
            bool SetFromCommandLine( const std :: string& strRealPath, std :: string& strError );
            bool SetByName( const std :: string& strName, std :: string& strError );
            const std :: string& GetPath( void ) const;
            bool IsFromCommandLine( void ) const;

            static bool IsValidName( const std :: string& strName, std :: string& strError );
            static bool UserDataFolder( const std :: string& strName, const EnvLookup& getEnv,
                                        std :: string& strFolder, std :: string& strError );
        };
    }
}

#endif /* __SAVEDIRECTORY_H__ */
