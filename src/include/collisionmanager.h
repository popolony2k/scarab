/*
 * collisionmanager.h
 *
 *  Created on: Jul 16, 2021
 *      Author: popolony2k
 */

#ifndef __COLLISIONMANAGER_H__
#define __COLLISIONMANAGER_H__


#include "collider.h"


class CollisionManager  {

    public:

    CollisionManager( void );
    virtual ~CollisionManager( void );

    void Update( void );
};

#endif /* __COLLISIONMANAGER_H__ */
