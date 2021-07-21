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

#define MAX_COLLIDER_LIST   255


class CollisionManager  {

    typedef std :: deque<Collider*>  ColliderList;
    typedef std :: array<ColliderList*, MAX_COLLIDER_LIST> ColliderLayerList;
    typedef std :: deque<ICollisionListener*> CollisionListenerList;
    typedef std :: pair<ColliderList*, ColliderList*> ColliderPair;
    typedef std :: deque<ColliderPair*> ColliderLayerRuleList;

    IWorld                   *m_pParentWorld;
    ColliderLayerList        m_ColliderLayerList;
    ColliderLayerRuleList    m_ColliderLayerRuleList;
    CollisionListenerList    m_Listeners;


    void FireOnCollision( Collider *pFirst, Collider *pSecond );

    public:

    CollisionManager( IWorld *pParentWorld );
    virtual ~CollisionManager( void );

    bool Add( Collider* pCollider, int nColliderLayerId );
    bool Remove( Collider *pCollider, int nColliderLayerId );
    void Clear( void );

    bool AddColliderLayerRule( int nFirstColliderLayerId,
                               int nSecondColliderLayerId );
    void AddCollisionListener( ICollisionListener *pListener );

    void Update( void );
};

#endif /* __COLLISIONMANAGER_H__ */
