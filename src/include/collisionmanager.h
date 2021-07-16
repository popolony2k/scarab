/*
 * collisionmanager.h
 *
 *  Created on: Jul 16, 2021
 *      Author: popolony2k
 */

#ifndef __COLLISIONMANAGER_H__
#define __COLLISIONMANAGER_H__


#include "collider.h"
#include "iworld.h"
#include <queue>


class CollisionManager  {

    typedef std :: deque<Collider*>  ColliderList;

    ColliderList      m_ColliderList;
    IWorld             *m_pParentWorld;

    public:

    CollisionManager( IWorld *pParentWorld );
    virtual ~CollisionManager( void );

    void Clear( void );

    void Update( void );
};

#endif /* __COLLISIONMANAGER_H__ */
