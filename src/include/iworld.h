/*
 * iworld.h
 *
 *  Created on: Jun 30, 2021
 *      Author: popolony2k
 */

#ifndef __IWORLD_H__
#define __IWORLD_H__

#include "worldbasedefs.h"
#include "zoomengine.h"


/**
 * World interface with basic operations inside the game world.
 */
class IWorld : public ZoomEngine  {

    public:

    virtual ~IWorld( void ) {};

    /**
     * Get last key from user input selected control.
     */
    virtual int GetKeyPressed( void ) = 0;

    /**
     * Reset the camera position.
     */
    virtual void ResetCamera( void ) = 0;

    /**
     * Move view camera up.
     */
    virtual void MoveCameraUp( void ) = 0;

    /**
     * Move view camera down.
     */
    virtual void MoveCameraDown( void ) = 0;

    /**
     * Move view camera left.
     */
    virtual void MoveCameraLeft( void ) = 0;

    /**
     * Move view camera right.
     */
    virtual void MoveCameraRight( void ) = 0;

    /**
     * Set layer parameters.
     * @param nLayerId The layer id to set layer parameters;
     * @param layer reference to layer parameters structure to set;
     */
    virtual bool SetLayer( int nLayerId, stLayer &layer ) = 0;

    /**
     * Set layer parameters.
     * @param szLayerName The layer name to set layer parameters;
     * @param layer reference to layer parameters structure to set;
     */
    virtual bool SetLayer( const char *szLayerName, stLayer &layer ) = 0;

    /**
     * Get the layer parameters.
     * @param nLayerId The layer id to get layer parameters;
     * @param layer reference to layer parameters structure to get;
     */
    virtual bool GetLayer( int nLayerId, stLayer &layer ) = 0;

    /**
     * Get the layer parameters.
     * @param szLayerName The layer name to get layer parameters;
     * @param layer reference to layer parameters structure to get;
     */
    virtual bool GetLayer( const char *szLayerName, stLayer &layer ) = 0;

    /**
     * Get a tile based row and column on specified map layer;
     * @param pos The tile position on layer map.
     * @param layer Reference to the layer whose tile will be
     * retrieved;
     * @param tile reference to @link stTile object to receive
     * tile information;
     */
    virtual bool GetTile( const stMatrixPosition& pos,
                          const stLayer& layer,
                          stTile& tile ) = 0;

    /**
     * Get the tile position based on world coordinate passed as parameter.
     * @param coord The World coordinate to translate to tile position;
     * @param pos Reference to struct @link stTilePosition to receive the
     * tile position based on world coordinate passed as parameter;
     */
    virtual bool WorldToTileMatrix( const stCoordinate2D& coord,
                                    stMatrixPosition& pos ) = 0;

    /**
     * Get the current map information data.
     * @param mapInfo Reference to the struct @link stMapInfo that will
     * receive the map information.
     */
    virtual bool GetMapInfo( stMapInfo& mapInfo ) = 0;
};

#endif /* __IWORLD_H__ */
