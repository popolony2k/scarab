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

#ifndef __SANDBOXFILE_H__
#define __SANDBOXFILE_H__

#include <cstddef>
#include <string>
#include <vector>


namespace Scarab  {
    namespace Engine  {

        /**
         * @brief A file's contents held in memory, read and written the way Lua's own file handles are
         * (plans/MASTER_PLAN.md item C8): the in-memory side of the sandbox's `io.open`. Lua's file handles
         * wrap a C FILE* opened straight from the operating system, which the sandbox must not do, and there
         * is no portable in-memory FILE* (none on Windows) - so a sandboxed file is read whole through the
         * file system when it is opened, changed here, and written back whole (atomically) by the Lua layer.
         *
         * Lua-agnostic and pure, so its behaviour is unit-tested against standard Lua's: the open modes
         * (`[rwa]+?b*`), `read( "l" / "L" / "a" / "n" / count )`, `write`, `seek` and append mode.
         */
        class SandboxFile  {

            public:

            /** @brief What an open mode allows, from Lua's mode string. */
            struct stMode  {
                bool  bRead;            // the file can be read
                bool  bWrite;           // the file can be written
                bool  bAppend;          // every write goes to the end ("a", "a+")
                bool  bTruncate;        // the file starts empty ("w", "w+")
                bool  bMustExist;       // opening fails when the file does not exist ("r", "r+")
            };

            /** @brief Where a seek counts from (Lua's "set", "cur" and "end"). */
            enum SeekWhence  {
                SEEK_WHENCE_SET,
                SEEK_WHENCE_CUR,
                SEEK_WHENCE_END,
                SEEK_WHENCE_LAST
            };

            private:

            std :: vector<char>  m_Data;
            std :: size_t        m_nPosition;
            stMode               m_Mode;
            bool                 m_bDirty;

            public:

            SandboxFile( void );

            static bool ParseMode( const std :: string& strMode, stMode& mode );

            void Open( const stMode& mode, const std :: vector<char>& existing );

            bool ReadLine( bool bKeepNewline, std :: string& strOut );
            std :: string ReadAll( void );
            bool ReadCount( std :: size_t nCount, std :: string& strOut );
            std :: string ReadNumberText( void );
            void Write( const char* pData, std :: size_t nSize );
            bool Seek( SeekWhence whence, long long nOffset, std :: size_t& nNewPosition );

            bool IsEndOfFile( void ) const;
            bool CanRead( void ) const;
            bool CanWrite( void ) const;
            bool IsDirty( void ) const;
            void MarkClean( void );
            const std :: vector<char>& GetData( void ) const;
            std :: size_t GetPosition( void ) const;
        };
    }
}

#endif /* __SANDBOXFILE_H__ */
