/*
 * collider.cpp
 *
 *  Created on: Jul 2, 2021
 *      Author: popolony2k
 */
#include "collider.h"


/**
 * Check if two rectangle areas are colliding;
 * @param fRect1X X coordinate of first rectangle;
 * @param fRect1Y Y coordinate of first rectangle;
 * @param fRect1Width Width of first rectangle;
 * @param fRect1Height Height of first rectangle;
 * @param fRect2X X coordinate of second rectangle;
 * @param fRect2Y Y coordinate of second rectangle;
 * @param fRect2Width Width of second rectangle;
 * @param fRect2Height Height of second rectangle;
 */
bool Collider :: RectRect( float fRect1X,
                           float fRect1Y,
                           float fRect1Width,
                           float fRect1Height,
                           float fRect2X,
                           float fRect2Y,
                           float fRect2Width,
                           float fRect2Height )  {

    if( ( ( fRect1X + fRect1Width ) >= fRect2X )  &&    // r1 right edge past r2 left
        ( fRect1X <= ( fRect2X + fRect2Width ) )  &&    // r1 left edge past r2 right
        ( ( fRect1Y + fRect1Height ) >= fRect2Y ) &&    // r1 top edge past r2 bottom
        ( fRect1Y <= ( fRect2Y + fRect2Height ) ) ) {   // r1 bottom edge past r2 top

        return true;
    }

    return false;
}


/**
 * Constructor. Initialize all class data.
 * @param Pointer to the dimension of object that owns this collider;
 */
Collider :: Collider( stDimension2D *pObjectArea )  {

    m_pObjectArea = pObjectArea;
}

/**
 * Destructor. Finalize all class data.
 */
Collider :: ~Collider( void )  {

}

/**
 * Check if collider object area has been hit by tile passed as parameter.
 * @param tile Reference to the tile struct containing all tile information;
 */
bool Collider :: Hit( stTile &tile )  {

    tmx_object  *pCollision = tile.pTile -> collision;
    bool        bHit = false;

    while( pCollision )  {

        switch( pCollision -> obj_type )  {
            case OT_SQUARE :
                bHit = RectRect( m_pObjectArea -> pos.x,
                                 m_pObjectArea -> pos.y,
                                 m_pObjectArea -> size.nWidth,
                                 m_pObjectArea -> size.nHeight,
                                 pCollision -> x,
                                 pCollision -> y,
                                 pCollision -> width,
                                 pCollision -> height );
                break;
            case OT_POLYGON :
                /* Still not supported */
                break;
            case OT_POLYLINE :
                /* Still not supported */
                break;
            case OT_ELLIPSE :
                /* Still not supported */
                break;
        }

        if( bHit )
            return true;
        else
            pCollision = tile.pTile -> collision -> next;
    }

    return bHit;
}

/**
 * Check if collider object area has been hit by a draw entity passed as
 * parameter.
 * @param tile Reference to the entity to be checked;
 */
bool Collider :: Hit( DrawEntity &entity )  {

    stDimension2D&   entityPos = entity.GetDimension2D();

    return RectRect( m_pObjectArea -> pos.x,
                     m_pObjectArea -> pos.y,
                     m_pObjectArea -> size.nWidth,
                     m_pObjectArea -> size.nHeight,
                     entityPos.pos.x,
                     entityPos.pos.y,
                     entityPos.size.nWidth,
                     entityPos.size.nHeight );
}
