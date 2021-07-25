/*
 * drawcanvas.h
 *
 *  Created on: Jul 7, 2021
 *      Author: popolony2k
 */

#ifndef __DRAWCANVAS_H__
#define __DRAWCANVAS_H__

#include "color.h"
#include "canvas.h"


class DrawCanvas : public Canvas {

    public:

    DrawCanvas( void );
    virtual ~DrawCanvas( void );

    /**
     * Must be implemented by children objects to provide
     * it's own draw behavior.
     */
    virtual void Update( void )  {};
};

#endif /* __DRAWCANVAS_H__ */
