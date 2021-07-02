/*
 * worldbasedefs.h
 *
 *  Created on: Jun 28, 2021
 *      Author: popolony2k
 */

#ifndef __WORLDBASEDEFS_H__
#define __WORLDBASEDEFS_H__

#include <tmx.h>


/**
 * 2D coordinate structs.
 */
struct stCoordinate2D  {
    int            x;
    int            y;
};

/**
 * 2D size struct.
 */
struct stSize2D  {
    int            nWidth;
    int            nHeight;
};

/**
 * 2D dimension struct;
 */
struct stDimension2D  {
    stCoordinate2D pos;
    stSize2D       size;
};

/**
 * Tile position struct.
 */
struct stTilePosition  {
    int            nTileRow;
    int            nTileCol;
};

/**
 * Layer definition struct.
 */
struct stLayer  {
    bool           bVisible;
    int            nOpacity;
    stCoordinate2D offset;
    tmx_layer      *pLayer;
};

/**
 * Tile definition.
 */
struct stTile  {
    uint32_t       nGID;    /* Tmx Global tile Identifier, uniquely identifies
                              * a tile at map scope.
                              * https://libtmx.readthedocs.io/en/latest/glossary.html#term-gid
                              */
    tmx_tile       *pTile;
};

/**
 * Map definition struct.
 */
struct stMapInfo  {
    stSize2D       mapSize;
    stSize2D       tileSize;
    tmx_map        *pMap;
};

#endif /* __WORLDBASEDEFS_H__ */
