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

#include "engine/savedirectory.h"
#include "filesystem/filesystemfactory.h"
#include <cstdlib>
#include <filesystem>

/** The characters a save directory's name may hold besides letters and digits. */
#define __NAME_EXTRA_CHARACTERS  "-_."


namespace Scarab  {
    namespace Engine  {

        /**
         * @brief Constructor: no save directory until the command line, the project file or the game names
         * one.
         */
        SaveDirectory :: SaveDirectory( void )  {

            m_bFromCommandLine = false;
        }

        /**
         * @brief Make a real folder the FileSystem's write directory (created if missing, mounted at the
         * virtual root ahead of every mount).
         *
         * @param strRealPath The real folder;
         * @param strError Receives the reason on failure;
         * @return true if it is the save directory now;
         */
        bool SaveDirectory :: Apply( const std :: string& strRealPath, std :: string& strError )  {

            SunLight :: FileSystem :: IFileSystem&  fileSystem = SunLight :: FileSystem :: FileSystemFactory :: GetFileSystem();

            UnmountExtras();

            if( !fileSystem.SetWriteDirectory( strRealPath ) )  {
                strError = "cannot use '" + strRealPath + "' as the save directory (not creatable or not writable)";

                // sunlight keeps the previous write directory when it refuses a new one: so do its mounts.
                if( !m_strPath.empty() )
                    MountExtras();

                return false;
            }

            m_strPath = strRealPath;

            if( !MountExtras() )  {
                strError = "cannot mount '" + strRealPath + "' as the save directory";
                return false;
            }

            return true;
        }

        /**
         * @brief Mount the save folder at its extra mount points (plans/MASTER_PLAN.md item C8): the private
         * one the save functions read through (appended - nothing else is mounted there), and the game's own
         * mount point (APP_DIR) ahead of every other mount, so an APP_DIR path finds a save first. The write
         * directory's own mount at the root is sunlight's (SetWriteDirectory).
         *
         * @return true if both mounts are in place;
         */
        bool SaveDirectory :: MountExtras( void )  {

            SunLight :: FileSystem :: IFileSystem&  fileSystem = SunLight :: FileSystem :: FileSystemFactory :: GetFileSystem();

            if( !fileSystem.Mount( m_strPath, SAVE_MOUNT_POINT, true ) )
                return false;

            m_strMountedPath = m_strPath;

            if( !m_strGameMountPoint.empty() && !fileSystem.Mount( m_strPath, m_strGameMountPoint, false ) )
                return false;

            m_strMountedGameMountPoint = m_strGameMountPoint;

            return true;
        }

        /**
         * @brief Remove the extra mounts MountExtras made (none when there are none).
         */
        void SaveDirectory :: UnmountExtras( void )  {

            SunLight :: FileSystem :: IFileSystem&  fileSystem = SunLight :: FileSystem :: FileSystemFactory :: GetFileSystem();

            if( m_strMountedPath.empty() )
                return;

            fileSystem.Unmount( m_strMountedPath, SAVE_MOUNT_POINT );

            if( !m_strMountedGameMountPoint.empty() )
                fileSystem.Unmount( m_strMountedPath, m_strMountedGameMountPoint );

            m_strMountedPath.clear();
            m_strMountedGameMountPoint.clear();
        }

        /**
         * @brief Name the virtual folder where the game's own files are mounted (APP_DIR), so the save folder
         * is mounted there too, ahead of them (plans/MASTER_PLAN.md item C8, decision 1b). Called once, before
         * any save directory is set.
         *
         * @param strMountPoint The virtual folder (APP_DIR);
         */
        void SaveDirectory :: SetGameMountPoint( const std :: string& strMountPoint )  {

            m_strGameMountPoint = strMountPoint;
        }

        /**
         * @brief Mount the save folder at the game's mount point again, ahead of every mount - after something
         * else was mounted there ahead of it (a game's .zip, EngineHost::ResolveEntryScript). Nothing to do
         * while there is no save directory.
         *
         * @param strError Receives the reason on failure;
         * @return true if the mounts are in place (or there is no save directory);
         */
        bool SaveDirectory :: Remount( std :: string& strError )  {

            if( m_strPath.empty() )
                return true;

            UnmountExtras();

            if( !MountExtras() )  {
                strError = "cannot mount '" + m_strPath + "' as the save directory";
                return false;
            }

            return true;
        }

        /**
         * @brief The virtual path a save of this name is read through: the private mount of the save folder
         * alone, so a game file of the same name is never taken for a save (plans/MASTER_PLAN.md item C8 -
         * before, a host path, which the sandbox refuses).
         *
         * @param strName The save's name, relative to the save folder;
         * @return The virtual path;
         */
        std :: string SaveDirectory :: GetSavePath( const std :: string& strName )  {

            return std :: string( SAVE_MOUNT_POINT ) + "/" + strName;
        }

        /**
         * @brief The command line's --save-dir: this real folder is the save directory, and a name given
         * later (project file, app_set_save_directory) no longer changes it.
         *
         * @param strRealPath The real folder, as the command line gives it;
         * @param strError Receives the reason on failure;
         * @return true if it is the save directory now;
         */
        bool SaveDirectory :: SetFromCommandLine( const std :: string& strRealPath, std :: string& strError )  {

            if( strRealPath.empty() )  {
                strError = "--save-dir needs a folder";
                return false;
            }

            if( !Apply( strRealPath, strError ) )
                return false;

            m_bFromCommandLine = true;

            return true;
        }

        /**
         * @brief Name the game's save folder: <the OS's per-user data folder>/<name>. With --save-dir on the
         * command line this succeeds and changes nothing - the command line wins (GetPath says where saves
         * go).
         *
         * @param strName The folder's name (see IsValidName);
         * @param strError Receives the reason on failure;
         * @return true if the save directory is set (by this name, or by --save-dir);
         */
        bool SaveDirectory :: SetByName( const std :: string& strName, std :: string& strError )  {

            if( !IsValidName( strName, strError ) )
                return false;

            if( m_bFromCommandLine )
                return true;

            std :: string  strFolder;

            if( !UserDataFolder( strName, []( const char* szName ) { return std :: getenv( szName ); }, strFolder, strError ) )
                return false;

            return Apply( strFolder, strError );
        }

        /**
         * @brief The real folder saves go into.
         *
         * @return The folder, or "" while there is none;
         */
        const std :: string& SaveDirectory :: GetPath( void ) const  {

            return m_strPath;
        }

        /**
         * @brief Whether the command line's --save-dir set the save directory.
         *
         * @return true if it did;
         */
        bool SaveDirectory :: IsFromCommandLine( void ) const  {

            return m_bFromCommandLine;
        }

        /**
         * @brief Whether a save directory's name is acceptable: one folder name, made of letters, digits and
         * "-_." only, not "." or ".." - so a game can only name a folder directly inside the per-user data
         * folder, never a path into another one.
         *
         * @param strName The name;
         * @param strError Receives the reason when it is not acceptable;
         * @return true if it is acceptable;
         */
        bool SaveDirectory :: IsValidName( const std :: string& strName, std :: string& strError )  {

            if( strName.empty() )  {
                strError = "a save directory's name can't be empty";
                return false;
            }

            if( ( strName == "." ) || ( strName == ".." ) )  {
                strError = "a save directory's name can't be '" + strName + "'";
                return false;
            }

            for( char c : strName )  {
                bool  bLetterOrDigit = ( ( c >= 'a' ) && ( c <= 'z' ) ) || ( ( c >= 'A' ) && ( c <= 'Z' ) ) || ( ( c >= '0' ) && ( c <= '9' ) );

                if( !bLetterOrDigit && ( std :: string( __NAME_EXTRA_CHARACTERS ).find( c ) == std :: string :: npos ) )  {
                    strError = "a save directory's name may hold only letters, digits and '" __NAME_EXTRA_CHARACTERS "' - '" + strName + "' doesn't";
                    return false;
                }
            }

            return true;
        }

        /**
         * @brief The OS's per-user data folder for a name: macOS ~/Library/Application Support/<name>,
         * Windows %APPDATA%\<name>, Linux and others $XDG_DATA_HOME/<name> (when set and absolute, as the XDG
         * spec says) or ~/.local/share/<name>. Pure apart from the environment, which getEnv reads (tests
         * pass their own).
         *
         * @param strName The folder's name (already validated);
         * @param getEnv Reads an environment variable;
         * @param strFolder Receives the folder;
         * @param strError Receives the reason on failure (the variable the OS needs is not set);
         * @return true if the folder could be worked out;
         */
        bool SaveDirectory :: UserDataFolder( const std :: string& strName, const EnvLookup& getEnv,
                                               std :: string& strFolder, std :: string& strError )  {

            std :: filesystem :: path  base;

#if defined( _WIN32 )
            const char*  szAppData = getEnv( "APPDATA" );

            if( !szAppData || !*szAppData )  {
                strError = "cannot find the per-user data folder: APPDATA is not set";
                return false;
            }

            base = szAppData;
#else
            const char*  szHome = getEnv( "HOME" );
#if defined( __APPLE__ )
            if( !szHome || !*szHome )  {
                strError = "cannot find the per-user data folder: HOME is not set";
                return false;
            }

            base = std :: filesystem :: path( szHome ) / "Library" / "Application Support";
#else
            const char*  szXdgDataHome = getEnv( "XDG_DATA_HOME" );

            if( szXdgDataHome && ( szXdgDataHome[0] == '/' ) )
                base = szXdgDataHome;
            else  {
                if( !szHome || !*szHome )  {
                    strError = "cannot find the per-user data folder: neither XDG_DATA_HOME nor HOME is set";
                    return false;
                }

                base = std :: filesystem :: path( szHome ) / ".local" / "share";
            }
#endif
#endif
            strFolder = ( base / strName ).string();

            return true;
        }
    }
}
