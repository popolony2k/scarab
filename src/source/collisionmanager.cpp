/*
 * collisionmanager.cpp
 *
 *  Created on: Jul 16, 2021
 *      Author: popolony2k
 */

#include "collisionmanager.h"



/**
 * Constructor. Initialize all class data.
 * @param pParentWorld Pointer to the world that this collision
 * manager is attached;
 */
CollisionManager :: CollisionManager( IWorld *pParentWorld )  {

    m_ColliderList.clear();
    m_pParentWorld = pParentWorld;
}

/**
 * Destructor. Finalize all class data.
 */
CollisionManager :: ~CollisionManager( void )  {

}

/**
 * Clear the collider manager object (lists status, ....
 */
void CollisionManager :: Clear( void )  {

    m_ColliderList.clear();
}

/**
 * Check if there are collisions between objects managed by
 * this collision manager.
 * Must be called every time is needed to check for all objects
 * collision.
 */
void CollisionManager :: Update( void )  {

    for( Collider *pCollider : m_ColliderList )  {
        // TODO: Check here
    }
}
