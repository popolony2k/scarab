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
    m_ColliderTypeArray[COLLIDER_MAIN_SPRITE]      = new ColliderList();
    m_ColliderTypeArray[COOLIDER_SECONDARY_SPRITE] = new ColliderList();
}

/**
 * Destructor. Finalize all class data.
 */
CollisionManager :: ~CollisionManager( void )  {

    Clear();
    m_Listeners.clear();
    delete m_ColliderTypeArray[COLLIDER_MAIN_SPRITE];
    delete m_ColliderTypeArray[COOLIDER_SECONDARY_SPRITE];
}

/**
 * Add a collider to manager;
 * @param pCollider Pointer to collider to add;
 * @param type Type of collider to add to the corresponding internal queue;
 */
void CollisionManager :: Add( Collider* pCollider, ColliderType type )  {

    ColliderList   *pColliderList = m_ColliderTypeArray[type];

    pColliderList -> push_back( pCollider );
}

/**
 * Add a collider from manager;
 * @param pCollider Pointer to collider to remove;
 * @param type Type of collider to add to the corresponding internal queue;
 */
void CollisionManager :: Remove( Collider *pCollider, ColliderType type )  {

    ColliderList   *pColliderList = m_ColliderTypeArray[type];
    ColliderList :: iterator itItem = find( pColliderList -> begin(),
                                            pColliderList -> end(),
                                            pCollider );

    if( itItem != pColliderList -> end() )
        pColliderList -> erase( itItem );
}

/**
 * Clear the collider manager object (lists status, ....
 */
void CollisionManager :: Clear( void )  {

    m_ColliderTypeArray[COLLIDER_MAIN_SPRITE] -> clear();
    m_ColliderTypeArray[COOLIDER_SECONDARY_SPRITE] -> clear();
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

    for( Collider *pFirst : *m_ColliderTypeArray[COLLIDER_MAIN_SPRITE] )  {
        for( Collider *pSecond : *m_ColliderTypeArray[COOLIDER_SECONDARY_SPRITE] )  {
            if( pFirst -> Hit( pSecond -> GetDimension() ) )  {
                FireOnCollision( pFirst, pSecond );
            }
        }
    }
}
