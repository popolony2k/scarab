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

            if( !SunLight :: FileSystem :: FileSystemFactory :: GetFileSystem().SetWriteDirectory( strRealPath ) )  {
                strError = "cannot use '" + strRealPath + "' as the save directory (not creatable or not writable)";
                return false;
            }

            m_strPath = strRealPath;

            return true;
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
