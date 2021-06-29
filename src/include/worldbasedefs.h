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
 * 2D coordinate struct.
 */
struct stCoordinate2D  {
    int    x;
    int    y;
};

/**
 * Layer definition struct.
 */
struct stLayer  {
    bool           bVisible;
    int            nOpacity;
    int            nOffsetX;
    int            nOffsetY;
    tmx_layer      *pLayer;
};

/**
 * Tile definition.
 */
struct stTile  {
    tmx_tile       *pTile;
    uint32_t       nGID;    /* Tmx Unique Graphical Id  */
};

/**
 * Map definition struct.
 */
struct stMapInfo  {
    unsigned int   nMapWidth;
    unsigned int   nMapHeight;
    unsigned int   nTileWidth;
    unsigned int   nTileHeight;
};

#endif /* __WORLDBASEDEFS_H__ */
