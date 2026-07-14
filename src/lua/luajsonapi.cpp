/*
 * luajsonapi.cpp
 *
 *  Created on: Jul 6, 2026
 *      Author: popolony2k
 */

#include "lua/luajsonapi.h"
#include <nlohmann/json.hpp>
#include <fstream>

extern "C"
{
  #include "lauxlib.h"
}


namespace Scarab  {
    namespace Engine  {
        namespace Lua  {

            /**
             * @brief Recursively push a JSON value onto the Lua stack as its
             * equivalent Lua value (a table for objects/arrays, a scalar otherwise).
             *
             * @param pLuaState Lua state to be used by engine call.
             * @param value The JSON value to convert and push;
             */
            static void PushJsonValue( lua_State *pLuaState, const nlohmann :: json &value )  {

                if( value.is_object() )  {
                    lua_newtable( pLuaState );

                    for( auto it = value.begin(); it != value.end(); ++it )  {
                        PushJsonValue( pLuaState, it.value() );
                        lua_setfield( pLuaState, -2, it.key().c_str() );
                    }
                }
                else if( value.is_array() )  {
                    lua_newtable( pLuaState );

                    int   nIndex = 1;

                    for( const nlohmann :: json &item : value )  {
                        PushJsonValue( pLuaState, item );
                        lua_rawseti( pLuaState, -2, nIndex++ );
                    }
                }
                else if( value.is_boolean() )
                    lua_pushboolean( pLuaState, value.get<bool>() );
                else if( value.is_number_integer() )
                    lua_pushinteger( pLuaState, value.get<lua_Integer>() );
                else if( value.is_number() )
                    lua_pushnumber( pLuaState, value.get<double>() );
                else if( value.is_string() )
                    lua_pushstring( pLuaState, value.get<std :: string>().c_str() );
                else
                    lua_pushnil( pLuaState );
            }

            /**
             * @brief Load a JSON file and return it's content as an equivalent Lua table.
             *
             * @param pLuaState Lua state to be used by engine call.
             * @return int number of return data (if any - required by lua engine)
             */
            int LuaJsonApi :: LoadJson( lua_State *pLuaState )  {

                if( lua_gettop( pLuaState ) != 1 )  {
                    fprintf( stderr, "Invalid number of arguments on load_json call.\n" );
                    lua_pushnil( pLuaState );

                    return 1;
                }

                const char       *szFileName = lua_tostring( pLuaState, 1 );
                std :: ifstream  fStream( szFileName );

                if( !fStream.is_open() )  {
                    fprintf( stderr, "load_json: Unable to open file [%s].\n", szFileName );
                    lua_pushnil( pLuaState );

                    return 1;
                }

                try  {
                    nlohmann :: json   jsonData = nlohmann :: json :: parse( fStream );

                    PushJsonValue( pLuaState, jsonData );
                }
                catch( const nlohmann :: json :: parse_error &ex )  {
                    fprintf( stderr, "load_json: Parse error on file [%s]: %s\n", szFileName, ex.what() );
                    lua_pushnil( pLuaState );
                }

                return 1;
            }

            /**
             * @brief Register the JSON bridge Lua-callable functions.
             *
             * @param pLuaState Lua state to be used by engine call.
             */
            void LuaJsonApi :: Register( lua_State *pLuaState )  {

                lua_register( pLuaState, "load_json", LuaJsonApi :: LoadJson );
            }
        }
    }
}
