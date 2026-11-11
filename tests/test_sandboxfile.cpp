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

/**
 * @brief The sandbox's in-memory file (plans/MASTER_PLAN.md item C8), checked against what standard
 * Lua 5.4's own file handles do: the open modes, every read format, writing, append mode and seek.
 */

#include <string>
#include <vector>
#include "doctest/doctest.h"
#include "engine/sandboxfile.h"

using namespace Scarab :: Engine;


/**
 * @brief Bytes from a string.
 * @param strText The text;
 * @return Its bytes;
 */
static std :: vector<char> Bytes( const std :: string& strText )  {

    return std :: vector<char>( strText.begin(), strText.end() );
}

/**
 * @brief A file opened in a mode on some contents.
 * @param strMode The Lua mode string (must be valid);
 * @param strContents What the file holds before it is opened;
 * @return The open file;
 */
static SandboxFile OpenFile( const std :: string& strMode, const std :: string& strContents )  {

    SandboxFile            file;
    SandboxFile :: stMode  mode;

    REQUIRE( SandboxFile :: ParseMode( strMode, mode ) );
    file.Open( mode, Bytes( strContents ) );

    return file;
}

/**
 * @brief A file's contents as a string.
 * @param file The file;
 * @return Its bytes as text;
 */
static std :: string Contents( const SandboxFile& file )  {

    return std :: string( file.GetData().begin(), file.GetData().end() );
}

TEST_CASE( "ParseMode accepts exactly Lua's modes" )  {

    SandboxFile :: stMode  mode;

    for( const char* szMode : { "r", "w", "a", "r+", "w+", "a+", "rb", "wb", "ab", "r+b", "w+b", "a+bb" } )
        CHECK_MESSAGE( SandboxFile :: ParseMode( szMode, mode ), szMode );

    for( const char* szMode : { "", "x", "rw", "+r", "r++", "rt", "r+t", "br", "wx" } )
        CHECK_MESSAGE( !SandboxFile :: ParseMode( szMode, mode ), szMode );
}

TEST_CASE( "ParseMode says what each mode allows" )  {

    SandboxFile :: stMode  mode;

    REQUIRE( SandboxFile :: ParseMode( "r", mode ) );
    CHECK( ( mode.bRead && !mode.bWrite && !mode.bAppend && !mode.bTruncate && mode.bMustExist ) );

    REQUIRE( SandboxFile :: ParseMode( "r+", mode ) );
    CHECK( ( mode.bRead && mode.bWrite && !mode.bAppend && !mode.bTruncate && mode.bMustExist ) );

    REQUIRE( SandboxFile :: ParseMode( "w", mode ) );
    CHECK( ( !mode.bRead && mode.bWrite && !mode.bAppend && mode.bTruncate && !mode.bMustExist ) );

    REQUIRE( SandboxFile :: ParseMode( "w+", mode ) );
    CHECK( ( mode.bRead && mode.bWrite && mode.bTruncate ) );

    REQUIRE( SandboxFile :: ParseMode( "a", mode ) );
    CHECK( ( !mode.bRead && mode.bWrite && mode.bAppend && !mode.bTruncate && !mode.bMustExist ) );

    REQUIRE( SandboxFile :: ParseMode( "a+", mode ) );
    CHECK( ( mode.bRead && mode.bWrite && mode.bAppend ) );
}

TEST_CASE( "Open truncates only in a w mode, and only then is a new file dirty" )  {

    SandboxFile  read = OpenFile( "r", "abc" );
    SandboxFile  write = OpenFile( "w", "abc" );
    SandboxFile  append = OpenFile( "a", "abc" );

    CHECK( Contents( read ) == "abc" );
    CHECK_FALSE( read.IsDirty() );
    CHECK( Contents( write ) == "" );
    CHECK( write.IsDirty() );
    CHECK( Contents( append ) == "abc" );
    CHECK_FALSE( append.IsDirty() );
}

TEST_CASE( "ReadLine: 'l' drops the newline, 'L' keeps it, the last line needs none" )  {

    SandboxFile    file = OpenFile( "r", "one\n\nthree" );
    std :: string  strLine;

    CHECK( file.ReadLine( false, strLine ) );
    CHECK( strLine == "one" );
    CHECK( file.ReadLine( false, strLine ) );
    CHECK( strLine == "" );
    CHECK( file.ReadLine( false, strLine ) );
    CHECK( strLine == "three" );
    CHECK_FALSE( file.ReadLine( false, strLine ) );

    SandboxFile  keep = OpenFile( "r", "a\nb" );

    CHECK( keep.ReadLine( true, strLine ) );
    CHECK( strLine == "a\n" );
    CHECK( keep.ReadLine( true, strLine ) );
    CHECK( strLine == "b" );
    CHECK_FALSE( keep.ReadLine( true, strLine ) );
}

TEST_CASE( "ReadAll never fails and returns empty at the end" )  {

    SandboxFile    file = OpenFile( "r", "line\nrest" );
    std :: string  strLine;

    REQUIRE( file.ReadLine( false, strLine ) );
    CHECK( file.ReadAll() == "rest" );
    CHECK( file.ReadAll() == "" );
    CHECK( file.IsEndOfFile() );
}

TEST_CASE( "ReadCount reads up to n bytes; 0 only tests for the end" )  {

    SandboxFile    file = OpenFile( "r", "abcde" );
    std :: string  strOut;

    CHECK( file.ReadCount( 0, strOut ) );
    CHECK( strOut == "" );
    CHECK( file.ReadCount( 3, strOut ) );
    CHECK( strOut == "abc" );
    CHECK( file.ReadCount( 10, strOut ) );
    CHECK( strOut == "de" );
    CHECK_FALSE( file.ReadCount( 0, strOut ) );
    CHECK_FALSE( file.ReadCount( 1, strOut ) );
}

TEST_CASE( "ReadNumberText scans a numeral as Lua's read_number does" )  {

    SandboxFile  file = OpenFile( "r", "  42 -3.5e+2 0x1F 0x1p4 .5 7abc" );

    CHECK( file.ReadNumberText() == "42" );
    CHECK( file.ReadNumberText() == "-3.5e+2" );
    CHECK( file.ReadNumberText() == "0x1F" );
    CHECK( file.ReadNumberText() == "0x1p4" );
    CHECK( file.ReadNumberText() == ".5" );
    CHECK( file.ReadNumberText() == "7" );

    std :: string  strRest;

    // The 'a' after 7 is not part of a decimal numeral, so it is left to read - as in Lua.
    CHECK( file.ReadCount( 3, strRest ) );
    CHECK( strRest == "abc" );
}

TEST_CASE( "ReadNumberText consumes what it accepted even when it is not a number" )  {

    SandboxFile  sign = OpenFile( "r", "-x" );

    // Lua reads "-", fails to convert it, and leaves the position after it.
    CHECK( sign.ReadNumberText() == "-" );
    CHECK( sign.GetPosition() == 1 );

    SandboxFile  exponent = OpenFile( "r", "e5" );

    // An exponent needs digits before it: Lua accepts nothing here.
    CHECK( exponent.ReadNumberText() == "" );
    CHECK( exponent.GetPosition() == 0 );

    SandboxFile  empty = OpenFile( "r", "   " );

    CHECK( empty.ReadNumberText() == "" );
    CHECK( empty.IsEndOfFile() );
}

TEST_CASE( "ReadNumberText gives up on a numeral longer than Lua's limit" )  {

    SandboxFile  file = OpenFile( "r", std :: string( 250, '9' ) );

    CHECK( file.ReadNumberText() == "" );
}

TEST_CASE( "Write overwrites at the position and grows the file" )  {

    SandboxFile  file = OpenFile( "r+", "abcdef" );

    file.Write( "XY", 2 );
    CHECK( Contents( file ) == "XYcdef" );
    CHECK( file.IsDirty() );

    std :: size_t  nPosition = 0;

    REQUIRE( file.Seek( SandboxFile :: SEEK_WHENCE_END, -1, nPosition ) );
    file.Write( "123", 3 );
    CHECK( Contents( file ) == "XYcde123" );

    file.MarkClean();
    CHECK_FALSE( file.IsDirty() );
}

TEST_CASE( "Append mode writes at the end whatever the position" )  {

    SandboxFile    file = OpenFile( "a+", "start" );
    std :: size_t  nPosition = 0;

    REQUIRE( file.Seek( SandboxFile :: SEEK_WHENCE_SET, 0, nPosition ) );
    file.Write( "-end", 4 );
    CHECK( Contents( file ) == "start-end" );

    REQUIRE( file.Seek( SandboxFile :: SEEK_WHENCE_SET, 0, nPosition ) );
    CHECK( file.ReadAll() == "start-end" );
}

TEST_CASE( "Seek counts from set, cur and end, and refuses before the start" )  {

    SandboxFile    file = OpenFile( "r", "0123456789" );
    std :: size_t  nPosition = 0;

    CHECK( file.Seek( SandboxFile :: SEEK_WHENCE_END, 0, nPosition ) );
    CHECK( nPosition == 10 );
    CHECK( file.Seek( SandboxFile :: SEEK_WHENCE_SET, 4, nPosition ) );
    CHECK( nPosition == 4 );
    CHECK( file.Seek( SandboxFile :: SEEK_WHENCE_CUR, 2, nPosition ) );
    CHECK( nPosition == 6 );
    CHECK( file.ReadAll() == "6789" );

    CHECK_FALSE( file.Seek( SandboxFile :: SEEK_WHENCE_CUR, -100, nPosition ) );
    CHECK( file.GetPosition() == 10 );
}

TEST_CASE( "A write past the end fills the gap with zeros" )  {

    SandboxFile    file = OpenFile( "w", "" );
    std :: size_t  nPosition = 0;

    REQUIRE( file.Seek( SandboxFile :: SEEK_WHENCE_SET, 3, nPosition ) );
    file.Write( "x", 1 );
    CHECK( file.GetData() == std :: vector<char>{ 0, 0, 0, 'x' } );
}
