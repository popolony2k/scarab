/*
 * collider.cpp
 *
 *  Created on: Jul 2, 2021
 *      Author: popolony2k
 */
#include "collider.h"
#include <cstring>


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
 */
Collider :: Collider( void )  {

    std :: memset( &m_Dimension, 0, sizeof( m_Dimension ) );
    m_pDimension = &m_Dimension;
}

/**
 * Destructor. Finalize all class data.
 */
Collider :: ~Collider( void )  {

}

/*
 * Add a pointer to a new parent dimension object passed as parameter;
 * @param pDimension Pointer to the new @link stDimension2D object
 * that will be used by this entity;
 */
void Collider :: SetDimensionPtr( stDimension2D *pDimension )  {

    m_pDimension = pDimension;
}

/**
 * Return the reference to the collider dimension data struct.
 */
stDimension2D& Collider :: GetDimension( void )  {

    return *m_pDimension;
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
                bHit = true;
// FIXME: considering whole tile as square, because it's x, y coordinates are not
// relative to map
//                bHit = RectRect( m_pDimension -> pos.x,
//                                 m_pDimension -> pos.y,
//                                 m_pDimension -> size.nWidth,
//                                 m_pDimension -> size.nHeight,
//                                 pCollision -> x,
//                                 pCollision -> y,
//                                 pCollision -> width,
//                                 pCollision -> height );
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
 * @param dimension Reference to a struct containing the area to be checked;
 */
bool Collider :: Hit( stDimension2D &dimension )  {

    return RectRect( m_pDimension -> pos.x,
                     m_pDimension -> pos.y,
                     m_pDimension -> size.nWidth,
                     m_pDimension -> size.nHeight,
                     dimension.pos.x,
                     dimension.pos.y,
                     dimension.size.nWidth,
                     dimension.size.nHeight );
}
