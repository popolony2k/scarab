/*
 * basecanvas.h
 *
 *  Created on: Jul 24, 2021
 *      Author: popolony2k
 */

#ifndef __BASECANVAS_H__
#define __BASECANVAS_H__

#include "viewport.h"
#include "baseentity.h"


class BaseCanvas : public BaseEntity  {

    BaseCanvas       *m_pParent;
    Viewport         *m_pViewport;
    Viewport         m_Viewport;


    public:

    BaseCanvas( void );
    virtual ~BaseCanvas( void );

    virtual void SetParent( BaseCanvas *pParent );
    BaseCanvas* GetParent( void );

    virtual void SetVisible( bool bVisible );
    virtual bool GetVisible( void );

    void SetViewport( Viewport *pViewport );
    Viewport& GetViewport( void );
};

#endif /* __BASECANVAS_H__ */
