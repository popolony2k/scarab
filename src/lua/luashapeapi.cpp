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

#include "lua/luashapeapi.h"
#include "lua/luaengineutil.h"
#include "drawsurface/ishapesurface.h"
#include "base/color.h"
#include <climits>
#include <cmath>

#define __DEFAULT_THICKNESS    1.0     // sunlight's default: a hairline
#define __DEFAULT_ROTATION     0.0     // degrees: unrotated
#define __DEFAULT_POINT_SIZE   1.0     // map units: one map unit square
#define __RGBA_CHANNELS        4
#define __OPTION_THICKNESS     "thickness"
#define __OPTION_ROTATION      "rotation"
#define __OPTION_NAMES         "thickness, rotation"


namespace Scarab  {
    namespace Engine  {
        namespace Lua  {

            /**
             * @brief Read a required number argument that must be finite (not NaN, not infinite) - a
             * shape's position or size.
             *
             * @param pLuaState Lua state to be used by engine call;
             * @param nIndex The argument's stack index;
             * @param szName The argument's name, for the message;
             * @param fOut Receives the number;
             * @param strError Receives the reason on failure;
             * @return true if the argument is a finite number;
             */
            static bool ReadFiniteNumberArg( lua_State* pLuaState, int nIndex, const char* szName, double& fOut, std :: string& strError )  {

                if( !LuaEngineUtil :: ReadNumberArg( pLuaState, nIndex, szName, fOut, strError ) )
                    return false;

                if( !std :: isfinite( fOut ) )  {
                    strError = std :: string( "argument '" ) + szName + "' must be a finite number";
                    return false;
                }

                return true;
            }

            /**
             * @brief Read a required size argument: a finite number, 0 or more.
             *
             * @param pLuaState Lua state to be used by engine call;
             * @param nIndex The argument's stack index;
             * @param szName The argument's name, for the message;
             * @param fOut Receives the size;
             * @param strError Receives the reason on failure;
             * @return true if the argument is a usable size;
             */
            static bool ReadSizeArg( lua_State* pLuaState, int nIndex, const char* szName, double& fOut, std :: string& strError )  {

                if( !ReadFiniteNumberArg( pLuaState, nIndex, szName, fOut, strError ) )
                    return false;

                if( fOut < 0 )  {
                    strError = std :: string( "argument '" ) + szName + "' must be 0 or more";
                    return false;
                }

                return true;
            }

            /**
             * @brief Read the four colour arguments r, g, b, a (each 0..255), like draw_filled_rectangle's.
             *
             * @param pLuaState Lua state to be used by engine call;
             * @param nFirst The stack index of r;
             * @param color Receives the colour;
             * @param strError Receives the reason on failure;
             * @return true if all four are integers in 0..255;
             */
            static bool ReadColorArgs( lua_State* pLuaState, int nFirst, SunLight :: Base :: stColor& color, std :: string& strError )  {

                static const char * const  aszChannel[__RGBA_CHANNELS] = { "r", "g", "b", "a" };

                lua_Integer  anChannel[__RGBA_CHANNELS] = { 0, 0, 0, 0 };

                for( int nArg = 0; nArg < __RGBA_CHANNELS; nArg++ )
                    if( !LuaEngineUtil :: ReadIntegerArgInRange( pLuaState, nFirst + nArg, aszChannel[nArg], 0, UCHAR_MAX, anChannel[nArg], strError ) )
                        return false;

                color.nRed   = ( unsigned char ) anChannel[0];
                color.nGreen = ( unsigned char ) anChannel[1];
                color.nBlue  = ( unsigned char ) anChannel[2];
                color.nAlpha = ( unsigned char ) anChannel[3];

                return true;
            }

            /**
             * @brief Read a flat point list { x1, y1, x2, y2, ... } (the form LÖVE uses), relative to the
             * shape's (x, y): a plain list of finite numbers, an even count, at least nMinPoints points.
             *
             * @param pLuaState Lua state to be used by engine call;
             * @param nIndex The list's stack index;
             * @param nMinPoints The fewest points the shape needs (MIN_POLYLINE_POINTS, MIN_POLYGON_POINTS);
             * @param points Receives the points;
             * @param strError Receives the reason on failure (it starts "argument 'points'");
             * @return true if the list is usable;
             */
            bool LuaShapeApi :: ReadPoints( lua_State* pLuaState, int nIndex, int nMinPoints,
                                            std :: vector<SunLight :: Base :: stVector2D>& points, std :: string& strError )  {

                if( !LuaEngineUtil :: CheckTableArg( pLuaState, nIndex, "points", strError ) )
                    return false;

                nIndex = lua_absindex( pLuaState, nIndex );

                lua_Integer  nCount = ( lua_Integer ) lua_rawlen( pLuaState, nIndex );
                lua_Integer  nKeys  = 0;

                lua_pushnil( pLuaState );

                while( lua_next( pLuaState, nIndex ) != 0 )  {
                    nKeys++;
                    lua_pop( pLuaState, 1 );
                }

                if( nKeys != nCount )  {
                    strError = "argument 'points' must be a plain list of numbers { x1, y1, x2, y2, ... }";
                    return false;
                }

                if( ( nCount % 2 ) != 0 )  {
                    strError = "argument 'points' must hold x, y pairs: an even count of numbers, got " + std :: to_string( nCount );
                    return false;
                }

                if( ( nCount / 2 ) < nMinPoints )  {
                    strError = "argument 'points' needs at least " + std :: to_string( nMinPoints ) + " points (" +
                               std :: to_string( nMinPoints * 2 ) + " numbers), got " + std :: to_string( nCount / 2 );
                    return false;
                }

                points.clear();
                points.reserve( ( size_t ) ( nCount / 2 ) );

                double  afPair[2] = { 0, 0 };

                for( lua_Integer nEntry = 1; nEntry <= nCount; nEntry++ )  {
                    lua_rawgeti( pLuaState, nIndex, nEntry );

                    bool    bNumber = ( lua_type( pLuaState, -1 ) == LUA_TNUMBER );
                    double  fValue  = bNumber ? lua_tonumber( pLuaState, -1 ) : 0;

                    lua_pop( pLuaState, 1 );

                    if( !bNumber || !std :: isfinite( fValue ) )  {
                        strError = "argument 'points' entry " + std :: to_string( nEntry ) + " must be a finite number";
                        return false;
                    }

                    afPair[( nEntry - 1 ) % 2] = fValue;

                    if( ( nEntry % 2 ) == 0 )
                        points.push_back( SunLight :: Base :: stVector2D { ( float ) afPair[0], ( float ) afPair[1] } );
                }

                return true;
            }

            /**
             * @brief Read the optional options table { thickness = 1, rotation = 0 }, strictly: absent or nil
             * gives the defaults; an unknown key, a thickness that is not a finite number above 0, or a
             * rotation that is not a finite number is refused.
             *
             * @param pLuaState Lua state to be used by engine call;
             * @param nIndex The table's stack index;
             * @param fThickness Receives the thickness, in map units;
             * @param fRotation Receives the rotation, in degrees clockwise;
             * @param strError Receives the reason on failure;
             * @return true if the options are usable;
             */
            bool LuaShapeApi :: ReadOptions( lua_State* pLuaState, int nIndex, double& fThickness, double& fRotation, std :: string& strError )  {

                fThickness = __DEFAULT_THICKNESS;
                fRotation  = __DEFAULT_ROTATION;

                if( lua_isnoneornil( pLuaState, nIndex ) )
                    return true;

                if( !LuaEngineUtil :: CheckTableArg( pLuaState, nIndex, "options", strError ) )
                    return false;

                nIndex = lua_absindex( pLuaState, nIndex );

                lua_pushnil( pLuaState );

                while( lua_next( pLuaState, nIndex ) != 0 )  {
                    bool           bStringKey = ( lua_type( pLuaState, -2 ) == LUA_TSTRING );
                    std :: string  strKey     = bStringKey ? lua_tostring( pLuaState, -2 ) : std :: string();
                    bool           bNumber    = ( lua_type( pLuaState, -1 ) == LUA_TNUMBER );
                    double         fValue     = bNumber ? lua_tonumber( pLuaState, -1 ) : 0;

                    lua_pop( pLuaState, 1 );

                    if( strKey == __OPTION_THICKNESS )  {
                        if( !bNumber || !std :: isfinite( fValue ) || ( fValue <= 0 ) )  {
                            strError = "option 'thickness' must be a number greater than 0";
                            lua_pop( pLuaState, 1 );
                            return false;
                        }

                        fThickness = fValue;
                    }
                    else if( strKey == __OPTION_ROTATION )  {
                        if( !bNumber || !std :: isfinite( fValue ) )  {
                            strError = "option 'rotation' must be a finite number";
                            lua_pop( pLuaState, 1 );
                            return false;
                        }

                        fRotation = fValue;
                    }
                    else  {
                        strError = bStringKey ? ( "unknown option '" + strKey + "' (the options are " __OPTION_NAMES ")" )
                                              : std :: string( "unknown option (the options are " __OPTION_NAMES ")" );
                        lua_pop( pLuaState, 1 );
                        return false;
                    }
                }

                return true;
            }

            /**
             * @brief Read the x, y, width, height arguments (1..4) of a rectangle or an ellipse box.
             *
             * @param pLuaState Lua state to be used by engine call;
             * @param afBox Receives x, y, width, height;
             * @param strError Receives the reason on failure;
             * @return true if all four are usable;
             */
            static bool ReadBoxArgs( lua_State* pLuaState, double ( &afBox )[4], std :: string& strError )  {

                return ReadFiniteNumberArg( pLuaState, 1, "x", afBox[0], strError ) &&
                       ReadFiniteNumberArg( pLuaState, 2, "y", afBox[1], strError ) &&
                       ReadSizeArg( pLuaState, 3, "width", afBox[2], strError ) &&
                       ReadSizeArg( pLuaState, 4, "height", afBox[3], strError );
            }

            /**
             * @brief Draw a rectangle outline in map units (sunlight's IShapeSurface::DrawRectangle).
             *
             * @param pLuaState Lua state to be used by engine call;
             * @return int number of return data (if any - required by lua engine)
             *
             * @luaname{shape_draw_rectangle(x, y, width, height, r, g, b, a, options) -> true | nil, message}
             * @luadoc
             * A rectangle outline with its top-left corner at `(x, y)`, in map units. `options` may
             * give a `thickness` and a `rotation` (clockwise, about `(x, y)`). `width` and `height`
             * must be `0` or more.
             * @luaexample
             * shape_draw_rectangle( 100, 80, 64, 32, 0, 255, 0, 255 )                              -- a hairline
             * shape_draw_rectangle( 100, 80, 64, 32, 0, 255, 0, 255, { thickness = 3, rotation = 30 } )
             */
            int LuaShapeApi :: DrawRectangle( lua_State* pLuaState )  {

                std :: string                 strError;
                double                        afBox[4] = { 0, 0, 0, 0 };
                double                        fThickness = __DEFAULT_THICKNESS;
                double                        fRotation  = __DEFAULT_ROTATION;
                SunLight :: Base :: stColor   color {};

                if( !ReadBoxArgs( pLuaState, afBox, strError ) || !ReadColorArgs( pLuaState, 5, color, strError ) ||
                    !ReadOptions( pLuaState, 9, fThickness, fRotation, strError ) )
                    return LuaEngineUtil :: PushError( pLuaState, strError );

                LuaEngineUtil :: GetShapeSurface( pLuaState ) -> DrawRectangle( afBox[0], afBox[1], afBox[2], afBox[3], color, fThickness, fRotation );

                return LuaEngineUtil :: PushTrue( pLuaState );
            }

            /**
             * @brief Draw an ellipse outline in map units, inscribed in a box (sunlight's
             * IShapeSurface::DrawEllipse).
             *
             * @param pLuaState Lua state to be used by engine call;
             * @return int number of return data (if any - required by lua engine)
             *
             * @luaname{shape_draw_ellipse(x, y, width, height, r, g, b, a, options) -> true | nil, message}
             * @luadoc
             * An ellipse outline inscribed in the box with its top-left corner at `(x, y)`, in map
             * units - a circle when `width` equals `height`. `options` may give a `thickness` and a
             * `rotation` (clockwise, about the box's centre).
             * @luaexample
             * -- a range ring of radius 96 around a tower
             * shape_draw_ellipse( tower.x - 96, tower.y - 96, 192, 192, 80, 200, 255, 255, { thickness = 2 } )
             */
            int LuaShapeApi :: DrawEllipse( lua_State* pLuaState )  {

                std :: string                 strError;
                double                        afBox[4] = { 0, 0, 0, 0 };
                double                        fThickness = __DEFAULT_THICKNESS;
                double                        fRotation  = __DEFAULT_ROTATION;
                SunLight :: Base :: stColor   color {};

                if( !ReadBoxArgs( pLuaState, afBox, strError ) || !ReadColorArgs( pLuaState, 5, color, strError ) ||
                    !ReadOptions( pLuaState, 9, fThickness, fRotation, strError ) )
                    return LuaEngineUtil :: PushError( pLuaState, strError );

                LuaEngineUtil :: GetShapeSurface( pLuaState ) -> DrawEllipse( afBox[0], afBox[1], afBox[2], afBox[3], color, fThickness, fRotation );

                return LuaEngineUtil :: PushTrue( pLuaState );
            }

            /**
             * @brief Read the arguments of a path (polyline or polygon): x, y, points, r, g, b, a, options.
             *
             * @param pLuaState Lua state to be used by engine call;
             * @param nMinPoints The fewest points the path needs;
             * @param fX Receives x;
             * @param fY Receives y;
             * @param points Receives the points;
             * @param color Receives the colour;
             * @param fThickness Receives the thickness;
             * @param fRotation Receives the rotation;
             * @param strError Receives the reason on failure;
             * @return true if every argument is usable;
             */
            static bool ReadPathArgs( lua_State* pLuaState, int nMinPoints, double& fX, double& fY,
                                      std :: vector<SunLight :: Base :: stVector2D>& points, SunLight :: Base :: stColor& color,
                                      double& fThickness, double& fRotation, std :: string& strError )  {

                return ReadFiniteNumberArg( pLuaState, 1, "x", fX, strError ) &&
                       ReadFiniteNumberArg( pLuaState, 2, "y", fY, strError ) &&
                       LuaShapeApi :: ReadPoints( pLuaState, 3, nMinPoints, points, strError ) &&
                       ReadColorArgs( pLuaState, 4, color, strError ) &&
                       LuaShapeApi :: ReadOptions( pLuaState, 8, fThickness, fRotation, strError );
            }

            /**
             * @brief Draw an open path in map units (sunlight's IShapeSurface::DrawPolyline).
             *
             * @param pLuaState Lua state to be used by engine call;
             * @return int number of return data (if any - required by lua engine)
             *
             * @luaname{shape_draw_polyline(x, y, points, r, g, b, a, options) -> true | nil, message}
             * @luagroup{shape_paths}
             * @luaheading{Paths}
             * @luadoc
             * An open path (`shape_draw_polyline`, at least 2 points) or a closed one
             * (`shape_draw_polygon`, at least 3 points, with an edge back to the first). `points` is a
             * flat list `{ x1, y1, x2, y2, ... }`, as LÖVE uses, relative to `(x, y)`, in map units.
             * `options` may give a `thickness` (a thick path has round joins; an open one has square
             * ends) and a `rotation` (clockwise, about `(x, y)`). A list with an odd count, too few
             * points, a hole or anything but finite numbers is refused, naming the entry.
             * @luaexample
             * shape_draw_polyline( 0, 0, { 10, 10, 120, 40, 200, 10 }, 255, 255, 0, 255 )
             * shape_draw_polygon( 300, 200, { 0, 0, 40, 0, 20, 30 }, 255, 0, 255, 255, { thickness = 2 } )   -- a triangle
             */
            int LuaShapeApi :: DrawPolyline( lua_State* pLuaState )  {

                std :: string                                  strError;
                double                                         fX = 0, fY = 0;
                std :: vector<SunLight :: Base :: stVector2D>  points;
                SunLight :: Base :: stColor                    color {};
                double                                         fThickness = __DEFAULT_THICKNESS;
                double                                         fRotation  = __DEFAULT_ROTATION;

                if( !ReadPathArgs( pLuaState, MIN_POLYLINE_POINTS, fX, fY, points, color, fThickness, fRotation, strError ) )
                    return LuaEngineUtil :: PushError( pLuaState, strError );

                LuaEngineUtil :: GetShapeSurface( pLuaState ) -> DrawPolyline( fX, fY, points.data(), ( int ) points.size(), color, fThickness, fRotation );

                return LuaEngineUtil :: PushTrue( pLuaState );
            }

            /**
             * @brief Draw a closed path in map units (sunlight's IShapeSurface::DrawPolygon).
             *
             * @param pLuaState Lua state to be used by engine call;
             * @return int number of return data (if any - required by lua engine)
             *
             * @luaname{shape_draw_polygon(x, y, points, r, g, b, a, options) -> true | nil, message}
             * @luagroup{shape_paths}
             */
            int LuaShapeApi :: DrawPolygon( lua_State* pLuaState )  {

                std :: string                                  strError;
                double                                         fX = 0, fY = 0;
                std :: vector<SunLight :: Base :: stVector2D>  points;
                SunLight :: Base :: stColor                    color {};
                double                                         fThickness = __DEFAULT_THICKNESS;
                double                                         fRotation  = __DEFAULT_ROTATION;

                if( !ReadPathArgs( pLuaState, MIN_POLYGON_POINTS, fX, fY, points, color, fThickness, fRotation, strError ) )
                    return LuaEngineUtil :: PushError( pLuaState, strError );

                LuaEngineUtil :: GetShapeSurface( pLuaState ) -> DrawPolygon( fX, fY, points.data(), ( int ) points.size(), color, fThickness, fRotation );

                return LuaEngineUtil :: PushTrue( pLuaState );
            }

            /**
             * @brief Draw a point - a filled square - in map units (sunlight's IShapeSurface::DrawPoint).
             *
             * @param pLuaState Lua state to be used by engine call;
             * @return int number of return data (if any - required by lua engine)
             *
             * @luaname{shape_draw_point(x, y, r, g, b, a, size) -> true | nil, message}
             * @luadoc
             * A filled square with its top-left corner at `(x, y)`, `size` map units wide (default
             * `1`, must be greater than `0`), at least one pixel at any zoom.
             * @luaexample
             * for i = 1, 5 do shape_draw_point( 100 + i * 20, 300, 255, 220, 0, 255, i ) end
             */
            int LuaShapeApi :: DrawPoint( lua_State* pLuaState )  {

                std :: string                 strError;
                double                        fX = 0, fY = 0;
                double                        fSize = __DEFAULT_POINT_SIZE;
                SunLight :: Base :: stColor   color {};

                if( !ReadFiniteNumberArg( pLuaState, 1, "x", fX, strError ) || !ReadFiniteNumberArg( pLuaState, 2, "y", fY, strError ) ||
                    !ReadColorArgs( pLuaState, 3, color, strError ) )
                    return LuaEngineUtil :: PushError( pLuaState, strError );

                if( !lua_isnoneornil( pLuaState, 7 ) )  {
                    if( !ReadFiniteNumberArg( pLuaState, 7, "size", fSize, strError ) )
                        return LuaEngineUtil :: PushError( pLuaState, strError );

                    if( fSize <= 0 )
                        return LuaEngineUtil :: PushError( pLuaState, "argument 'size' must be greater than 0" );
                }

                LuaEngineUtil :: GetShapeSurface( pLuaState ) -> DrawPoint( fX, fY, color, fSize );

                return LuaEngineUtil :: PushTrue( pLuaState );
            }

            /**
             * @brief Register the shape functions. They need the renderer only once their arguments are
             * checked.
             *
             * @param pLuaState Lua state to be used by engine call;
             */
            void LuaShapeApi :: Register( lua_State* pLuaState )  {

                lua_register( pLuaState, "shape_draw_rectangle", LuaShapeApi :: DrawRectangle );
                lua_register( pLuaState, "shape_draw_ellipse", LuaShapeApi :: DrawEllipse );
                lua_register( pLuaState, "shape_draw_polyline", LuaShapeApi :: DrawPolyline );
                lua_register( pLuaState, "shape_draw_polygon", LuaShapeApi :: DrawPolygon );
                lua_register( pLuaState, "shape_draw_point", LuaShapeApi :: DrawPoint );
            }
        }
    }
}
