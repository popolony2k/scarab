/*
 * mapstruct.h
 *
 *  Created on: Jun 28, 2021
 *      Author: popolony2k
 */

#ifndef __MAPDEFS_H__
#define __MAPDEFS_H__

/**
 * Layer definition struct.
 */
struct stLayer  {
    bool      bVisible;
    int       nOpacity;
    int       nOffsetX;
    int       nOffsetY;
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

#endif /* __MAPDEFS_H__ */
