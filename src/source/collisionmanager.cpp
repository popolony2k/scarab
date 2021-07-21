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
bool CollisionManager :: Add( Collider* pCollider, int nColliderLayerId )  {

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
bool CollisionManager :: Remove( Collider *pCollider, int nColliderLayerId )  {

    if( nColliderLayerId < m_ColliderLayerList.size() )  {
        ColliderList   *pColliderList = m_ColliderLayerList[nColliderLayerId];
        ColliderList :: iterator itItem = find( pColliderList -> begin(),
                                                pColliderList -> end(),
                                                pCollider );

        if( itItem != pColliderList -> end() )
            pColliderList -> erase( itItem );

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

    for( ColliderPair *pPair : m_ColliderLayerRuleList )  {
        delete pPair;
    }
}

/**
 * Add collider checking rule. This method pair two layer that will be checked
 * in collision update checking.
 * @param nFirstColliderLayerId First layer to be checked;
 * @param nSecondColliderLayerId second layer to be checked;
 */
bool CollisionManager :: AddColliderLayerRule( int nFirstColliderLayerId,
                                               int nSecondColliderLayerId )  {

    if( ( nFirstColliderLayerId < m_ColliderLayerList.size() ) &&
        ( nFirstColliderLayerId < m_ColliderLayerList.size() ) )  {

        ColliderPair  *pPair = new ColliderPair();

        pPair -> first  = m_ColliderLayerList[nFirstColliderLayerId];
        pPair -> second = m_ColliderLayerList[nSecondColliderLayerId];

        m_ColliderLayerRuleList.push_back( pPair );

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

    for( ColliderPair *pPair : m_ColliderLayerRuleList )  {
        for( Collider *pFirst : *pPair -> first )  {
            for( Collider *pSecond : *pPair -> second )  {
                if( pFirst -> Hit( pSecond -> GetDimension() ) )  {
                    FireOnCollision( pFirst, pSecond );
                }
            }
        }
    }
}
