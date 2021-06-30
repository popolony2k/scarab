/*
 * worldlistener.h
 *
 *  Created on: Jun 30, 2021
 *      Author: popolony2k
 */

#ifndef __IWORLDLISTENER_H__
#define __IWORLDLISTENER_H__

#include "iworld.h"


class IWorldListener  {

    public:

    virtual ~IWorldListener( void ) {};

    /**
     * Must be implemented to provide action on frame update.
     * @param world The @link IWorld object used to interact with world
     * renderer implementation;
     */
    virtual void OnUpdate( IWorld& world ) = 0;
};

#endif /* __IWORLDLISTENER_H__ */
