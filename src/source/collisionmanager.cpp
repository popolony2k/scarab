/*
 * collisionmanager.cpp
 *
 *  Created on: Jul 16, 2021
 *      Author: popolony2k
 */

#include <algorithm>
#include "collisionmanager.h"


/**
 * Throw the OnCollision event through all registered listeners.
 * @param pFirst The first collider involved in the collision;
 * @param pSecond The second collider involved in the collision;
 */
void CollisionManager :: FireOnCollision( Collider *pFirst, Collider *pSecond )  {

    for( ICollisionListener *pListener : m_Listeners )  {
        pListener -> OnCollision( pFirst, pSecond );
    }
}

/**
 * Throw the OnCollision event through all registered listeners.
 * @param pFirst The collider involved in the collision;
 * @param pSecond The layer tile object involved in the collision;
 */
void CollisionManager :: FireOnCollision( Collider *pFirst, stTile* pSecond )  {

    for( ICollisionListener *pListener : m_Listeners )  {
        pListener -> OnCollision( pFirst, pSecond );
    }
}

/**
 * Constructor. Initialize all class data.
 * @param pParentWorld Pointer to the world that this collision
 * manager is attached;
 */
CollisionManager :: CollisionManager( IWorld *pParentWorld )  {

    m_pParentWorld = pParentWorld;

    for( int nCount = 0; nCount < m_ColliderLayerList.size(); nCount++)  {
        m_ColliderLayerList[nCount] = new ColliderList();
    }
}

/**
 * Destructor. Finalize all class data.
 */
CollisionManager :: ~CollisionManager( void )  {

    Clear();
    m_Listeners.clear();
}

/**
 * Add a collider to manager;
 * @param pCollider Pointer to collider to add;
 * @param nColliderLayerId collider layer id to add the collider;
 */
bool CollisionManager :: AddCollider( Collider* pCollider,
                                      int nColliderLayerId )  {

    if( nColliderLayerId < m_ColliderLayerList.size() )  {
        m_ColliderLayerList[nColliderLayerId] -> push_back( pCollider );

        return true;
    }

    return false;
}

/**
 * Add a collider from manager;
 * @param pCollider Pointer to collider to remove;
 * @param nColliderLayerId collider layer id to remove the collider;
 */
bool CollisionManager :: RemoveCollider( Collider *pCollider,
                                         int nColliderLayerId )  {

    if( nColliderLayerId < m_ColliderLayerList.size() )  {
        ColliderList   *pColliderList = m_ColliderLayerList[nColliderLayerId];
        ColliderList :: iterator itItem = std :: find( pColliderList -> begin(),
                                                       pColliderList -> end(),
                                                       pCollider );

        if( itItem != pColliderList -> end() )
            pColliderList -> erase( itItem );

        return true;
    }

    return false;
}

/**
 * Remove all colliders from layer.
 * @param nColliderLayerId collider layer id to remove the all colliders.
 * If this parameter is -1 (default), remove all collider from all layers;
 */
bool CollisionManager :: RemoveAll( int nColliderLayerId )  {

    if( nColliderLayerId < m_ColliderLayerList.size() )  {
        if( nColliderLayerId < 0 )  {
            for( ColliderList *pColliderList : m_ColliderLayerList )  {
                pColliderList -> clear();
            }
        }
        else  {
            m_ColliderLayerList[nColliderLayerId] -> clear();
        }

        return true;
    }

    return false;
}

/**
 * Clear the collider manager object (lists status, ....
 */
void CollisionManager :: Clear( void )  {

    for( int nCount = 0; nCount < m_ColliderLayerList.size(); nCount++)  {
        delete m_ColliderLayerList[nCount];
    }

    for( ColliderPair *pPair : m_ColliderToColliderRuleList )  {
        delete pPair;
    }

    for( ColliderTileLayerPair *pPair : m_ColliderToTileLayerRuleList )  {
        delete pPair;
    }
}

/**
 * Add collider to collider checking rule based on it's layer id.
 * This method pair two layer that will be checked in collision update
 * checking.
 * @param nFirstColliderLayerId First collider layer id that will be added to
 * checking rule;;
 * @param nSecondColliderLayerId Second collider layer id that will be added to
 * checking rule;
 */
bool CollisionManager :: AddColliderToColliderRule( int nFirstColliderLayerId,
                                                    int nSecondColliderLayerId )  {

    if( ( nFirstColliderLayerId < m_ColliderLayerList.size() ) &&
        ( nFirstColliderLayerId < m_ColliderLayerList.size() ) )  {

        ColliderPair  *pPair = new ColliderPair();

        pPair -> first  = m_ColliderLayerList[nFirstColliderLayerId];
        pPair -> second = m_ColliderLayerList[nSecondColliderLayerId];

        m_ColliderToColliderRuleList.push_back( pPair );

        return true;
    }

    return false;
}

/**
 * Add collider to tile checking rule based on it's layer id.
 * This method pair two layer that will be checked in collision update
 * checking.
 * @param nColliderId The collider layer id that will be added to
 * checking rule;
 * @param nLayerId The tile layer id that will be added to checking rule;
 */
bool CollisionManager :: AddColliderToTileRule( int nColliderLayerId,
                                                int nTileLayerId )  {

    stLayer layer;

    if( ( nColliderLayerId < m_ColliderLayerList.size() ) &&
        m_pParentWorld -> GetLayer( nTileLayerId, layer ) )  {

        ColliderTileLayerPair  *pPair = new ColliderTileLayerPair();

        pPair -> first  = m_ColliderLayerList[nColliderLayerId];
        pPair -> second = nTileLayerId;

        m_ColliderToTileLayerRuleList.push_back( pPair );

        return true;
    }

    return false;
}

/**
 * Add an ICollisionListener event object to manager;
 * @param pListener Pointer to the listener object to add;
 */
void CollisionManager :: AddCollisionListener( ICollisionListener *pListener )  {

    m_Listeners.push_back( pListener );
}

/**
 * Check if there are collisions between objects managed by
 * this collision manager.
 * Must be called every time is needed to check for all objects
 * collision.
 */
void CollisionManager :: Update( void )  {

    /*
     * Check collisions between colliders only.
     */
    for( ColliderPair *pPair : m_ColliderToColliderRuleList )  {
        for( Collider *pFirst : *pPair -> first )  {
            for( Collider *pSecond : *pPair -> second )  {
                if( pFirst -> Hit( pSecond -> GetDimension2D() ) )  {
                    FireOnCollision( pFirst, pSecond );
                }
            }
        }
    }

    /*
     * Check collisions between colliders against static
     * layer objects defined as collision on layer map.
     */
    for( ColliderTileLayerPair *pPair : m_ColliderToTileLayerRuleList )  {
        for( Collider *pFirst : *pPair -> first )  {
            stTile      tile;
            stLayer     layer;

            if( m_pParentWorld -> GetLayer( pPair -> second, layer ) )  {
                stDimension2D&    spritePos = pFirst -> GetDimension2D();
                stMatrixPosition  tilePos   = { 0, 0 };

                if( m_pParentWorld -> WorldToTileMatrix( spritePos.pos, tilePos ) ) {
                    if( m_pParentWorld -> GetTile( tilePos, layer, tile ) &&
                        pFirst -> Hit( tile ) )  {
                        FireOnCollision(pFirst, &tile );
                    }
                }
            }
        }
    }
}
