/*
 * collisionmanager.h
 *
 *  Created on: Jul 16, 2021
 *      Author: popolony2k
 */

#ifndef __COLLISIONMANAGER_H__
#define __COLLISIONMANAGER_H__


#include "icollisionlistener.h"
#include "iworld.h"
#include <queue>
#include <array>

/**
 *
 */
enum ColliderType  {
    COLLIDER_MAIN_SPRITE = 0,
    COOLIDER_SECONDARY_SPRITE
};

class CollisionManager  {

    typedef std :: deque<Collider*>  ColliderList;
    typedef std :: array<ColliderList*, 2> ColliderTypeArray;
    typedef std :: deque<ICollisionListener*> CollisionListenerList;

    IWorld                   *m_pParentWorld;
    ColliderTypeArray        m_ColliderTypeArray;
    CollisionListenerList    m_Listeners;


    void FireOnCollision( Collider *pFirst, Collider *pSecond );

    public:

    CollisionManager( IWorld *pParentWorld );
    virtual ~CollisionManager( void );

    void Add( Collider* pCollider, ColliderType type );
    void Remove( Collider *pCollider, ColliderType type );
    void Clear( void );

    void AddCollisionListener( ICollisionListener *pListener );

    void Update( void );
};

#endif /* __COLLISIONMANAGER_H__ */
