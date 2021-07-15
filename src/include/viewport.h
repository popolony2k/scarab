/*
 * iviewport.h
 *
 *  Created on: Jul 14, 2021
 *      Author: popolony2k
 */

#ifndef __VIEWPORT_H__
#define __VIEWPORT_H__

#include "worldbasedefs.h"


class Viewport  {

    protected:

    stViewport          m_Viewport;


    public:

    Viewport( void );
    virtual ~Viewport( void );

    void SetViewport( stViewport viewport );
    stViewport GetViewport( void );
};

#endif /* __VIEWPORT_H__ */
