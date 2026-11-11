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

#include "engine/sandboxfile.h"
#include <algorithm>
#include <cctype>
#include <cstring>

/** The longest numeral read( "n" ) reads, as Lua's own L_MAXLENNUM (liolib.c): a longer one is not a number. */
#define __MAX_NUMERAL_LENGTH  200
/** The characters that may follow the first one of a mode string (Lua's L_MODEEXT). */
#define __MODE_EXTENSIONS     "b"


namespace Scarab  {
    namespace Engine  {

        /**
         * @brief Reads a numeral the way Lua's read_number does (liolib.c), one character at a time: it keeps
         * every character it accepts, and gives up (an empty result) past __MAX_NUMERAL_LENGTH.
         */
        class NumeralReader  {

            const std :: vector<char>&  m_Data;
            std :: size_t&              m_nPosition;
            std :: string               m_strBuffer;
            bool                        m_bOverflow;

            public:

            /**
             * @brief Constructor.
             *
             * @param data The file's contents;
             * @param nPosition The read position, moved past every character accepted;
             */
            NumeralReader( const std :: vector<char>& data, std :: size_t& nPosition ) : m_Data( data ), m_nPosition( nPosition )  {

                m_bOverflow = false;
            }

            /**
             * @brief The current character, or -1 at the end of the file.
             *
             * @return The character, as an unsigned char value;
             */
            int Current( void ) const  {

                return ( m_nPosition < m_Data.size() ) ? ( unsigned char ) m_Data[m_nPosition] : -1;
            }

            /**
             * @brief Keep the current character and move to the next one (Lua's nextc).
             *
             * @return false once the numeral is too long (the result is then invalid);
             */
            bool Next( void )  {

                if( m_strBuffer.size() >= __MAX_NUMERAL_LENGTH )  {
                    m_bOverflow = true;
                    return false;
                }

                m_strBuffer.push_back( ( char ) Current() );
                m_nPosition++;

                return true;
            }

            /**
             * @brief Accept the current character if it is one of two (Lua's test2).
             *
             * @param cFirst One character;
             * @param cSecond The other;
             * @return true if it was accepted;
             */
            bool Test2( char cFirst, char cSecond )  {

                int  c = Current();

                return ( ( c == ( unsigned char ) cFirst ) || ( c == ( unsigned char ) cSecond ) ) && Next();
            }

            /**
             * @brief Accept a run of digits, hexadecimal or decimal (Lua's readdigits).
             *
             * @param bHex Whether hexadecimal digits are accepted;
             * @return How many were accepted;
             */
            int ReadDigits( bool bHex )  {

                int  nCount = 0;

                while( ( Current() >= 0 ) && ( bHex ? std :: isxdigit( Current() ) : std :: isdigit( Current() ) ) && Next() )
                    nCount++;

                return nCount;
            }

            /**
             * @brief The characters read, or "" when the numeral was too long.
             *
             * @return The numeral's text;
             */
            std :: string Result( void ) const  {

                return m_bOverflow ? std :: string() : m_strBuffer;
            }
        };

        /**
         * @brief Constructor: an empty, unreadable and unwritable file until Open.
         */
        SandboxFile :: SandboxFile( void )  {

            m_nPosition = 0;
            m_Mode      = stMode { false, false, false, false, false };
            m_bDirty    = false;
        }

        /**
         * @brief Read a Lua mode string - `[rwa]+?b*`, exactly what Lua's own io.open accepts (l_checkmode).
         *
         * @param strMode The mode string;
         * @param mode Receives what it allows;
         * @return false for a mode Lua would refuse;
         */
        bool SandboxFile :: ParseMode( const std :: string& strMode, stMode& mode )  {

            if( strMode.empty() || ( std :: strchr( "rwa", strMode[0] ) == nullptr ) )
                return false;

            std :: size_t  nRest = 1;
            bool           bPlus = ( strMode.size() > 1 ) && ( strMode[1] == '+' );

            if( bPlus )
                nRest++;

            if( strMode.find_first_not_of( __MODE_EXTENSIONS, nRest ) != std :: string :: npos )
                return false;

            mode.bRead      = ( strMode[0] == 'r' ) || bPlus;
            mode.bWrite     = ( strMode[0] != 'r' ) || bPlus;
            mode.bAppend    = ( strMode[0] == 'a' );
            mode.bTruncate  = ( strMode[0] == 'w' );
            mode.bMustExist = ( strMode[0] == 'r' );

            return true;
        }

        /**
         * @brief Open the file: its mode, and the contents it starts with (none in a truncating mode), at
         * position 0. Nothing is dirty yet - except a new file in a writing mode, which closing creates.
         *
         * @param mode The mode (@see ParseMode);
         * @param existing The file's current contents, empty when it does not exist;
         */
        void SandboxFile :: Open( const stMode& mode, const std :: vector<char>& existing )  {

            m_Mode      = mode;
            m_Data      = mode.bTruncate ? std :: vector<char>() : existing;
            m_nPosition = 0;
            m_bDirty    = mode.bTruncate;
        }

        /**
         * @brief Read a line (Lua's "l", or "L" keeping the newline).
         *
         * @param bKeepNewline Whether the newline is kept ("L");
         * @param strOut Receives the line;
         * @return false at the end of the file (nothing read);
         */
        bool SandboxFile :: ReadLine( bool bKeepNewline, std :: string& strOut )  {

            strOut.clear();

            if( m_nPosition >= m_Data.size() )
                return false;

            auto  itBegin   = m_Data.begin() + ( std :: ptrdiff_t ) m_nPosition;
            auto  itNewline = std :: find( itBegin, m_Data.end(), '\n' );

            strOut.assign( itBegin, itNewline );
            m_nPosition = ( std :: size_t ) ( itNewline - m_Data.begin() );

            if( itNewline != m_Data.end() )  {
                m_nPosition++;

                if( bKeepNewline )
                    strOut.push_back( '\n' );
            }

            return true;
        }

        /**
         * @brief Read the rest of the file (Lua's "a": never fails, "" at the end).
         *
         * @return What is left;
         */
        std :: string SandboxFile :: ReadAll( void )  {

            std :: string  strOut;

            if( m_nPosition < m_Data.size() )
                strOut.assign( m_Data.begin() + ( std :: ptrdiff_t ) m_nPosition, m_Data.end() );

            m_nPosition = m_Data.size();

            return strOut;
        }

        /**
         * @brief Read up to nCount bytes (Lua's numeric format). A count of 0 reads nothing and tells whether
         * the end of the file was reached, as Lua's test_eof does.
         *
         * @param nCount How many bytes;
         * @param strOut Receives them;
         * @return false at the end of the file;
         */
        bool SandboxFile :: ReadCount( std :: size_t nCount, std :: string& strOut )  {

            strOut.clear();

            if( m_nPosition >= m_Data.size() )
                return false;

            std :: size_t  nTake = std :: min( nCount, m_Data.size() - m_nPosition );

            strOut.assign( m_Data.begin() + ( std :: ptrdiff_t ) m_nPosition, m_Data.begin() + ( std :: ptrdiff_t ) ( m_nPosition + nTake ) );
            m_nPosition += nTake;

            return true;
        }

        /**
         * @brief Read a numeral as Lua's read_number does (liolib.c): skip blanks, then accept a sign, "0x",
         * digits, a decimal point, digits and an exponent. The characters accepted are consumed even when
         * they turn out not to be a number - the caller converts the text (lua_stringtonumber), as Lua does.
         *
         * @return The numeral's text ("" when there is none, or it is too long);
         */
        std :: string SandboxFile :: ReadNumberText( void )  {

            while( ( m_nPosition < m_Data.size() ) && std :: isspace( ( unsigned char ) m_Data[m_nPosition] ) )
                m_nPosition++;

            NumeralReader  reader( m_Data, m_nPosition );
            int            nCount = 0;
            bool           bHex   = false;

            reader.Test2( '-', '+' );

            if( reader.Test2( '0', '0' ) )  {
                if( reader.Test2( 'x', 'X' ) )
                    bHex = true;
                else
                    nCount = 1;
            }

            nCount += reader.ReadDigits( bHex );

            if( reader.Test2( '.', '.' ) )
                nCount += reader.ReadDigits( bHex );

            if( ( nCount > 0 ) && ( bHex ? reader.Test2( 'p', 'P' ) : reader.Test2( 'e', 'E' ) ) )  {
                reader.Test2( '-', '+' );
                reader.ReadDigits( false );
            }

            return reader.Result();
        }

        /**
         * @brief Write bytes at the position (at the end in append mode), growing the file as needed.
         *
         * @param pData The bytes;
         * @param nSize How many;
         */
        void SandboxFile :: Write( const char* pData, std :: size_t nSize )  {

            if( m_Mode.bAppend )
                m_nPosition = m_Data.size();

            if( m_nPosition + nSize > m_Data.size() )
                m_Data.resize( m_nPosition + nSize );

            std :: copy( pData, pData + nSize, m_Data.begin() + ( std :: ptrdiff_t ) m_nPosition );
            m_nPosition += nSize;
            m_bDirty     = true;
        }

        /**
         * @brief Move the position (Lua's file:seek). A position past the end is allowed, as with a C file; a
         * write there fills the gap with zeros.
         *
         * @param whence Where the offset counts from;
         * @param nOffset The offset;
         * @param nNewPosition Receives the new position;
         * @return false when the position would be before the start (Lua then reports "Invalid argument");
         */
        bool SandboxFile :: Seek( SeekWhence whence, long long nOffset, std :: size_t& nNewPosition )  {

            long long  nBase = ( whence == SEEK_WHENCE_SET ) ? 0 :
                               ( whence == SEEK_WHENCE_CUR ) ? ( long long ) m_nPosition : ( long long ) m_Data.size();

            if( nBase + nOffset < 0 )
                return false;

            m_nPosition  = ( std :: size_t ) ( nBase + nOffset );
            nNewPosition = m_nPosition;

            return true;
        }

        /**
         * @brief Whether the position is at (or past) the end.
         *
         * @return true at the end;
         */
        bool SandboxFile :: IsEndOfFile( void ) const  {

            return m_nPosition >= m_Data.size();
        }

        /**
         * @brief Whether the mode allows reading.
         *
         * @return true if it does;
         */
        bool SandboxFile :: CanRead( void ) const  {

            return m_Mode.bRead;
        }

        /**
         * @brief Whether the mode allows writing.
         *
         * @return true if it does;
         */
        bool SandboxFile :: CanWrite( void ) const  {

            return m_Mode.bWrite;
        }

        /**
         * @brief Whether there is something to write back (changed since it was opened or last flushed).
         *
         * @return true if there is;
         */
        bool SandboxFile :: IsDirty( void ) const  {

            return m_bDirty;
        }

        /**
         * @brief Record that the contents were written back.
         */
        void SandboxFile :: MarkClean( void )  {

            m_bDirty = false;
        }

        /**
         * @brief The file's contents.
         *
         * @return The bytes;
         */
        const std :: vector<char>& SandboxFile :: GetData( void ) const  {

            return m_Data;
        }

        /**
         * @brief The position.
         *
         * @return The byte offset;
         */
        std :: size_t SandboxFile :: GetPosition( void ) const  {

            return m_nPosition;
        }
    }
}
