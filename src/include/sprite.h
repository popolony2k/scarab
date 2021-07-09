/*
 * sprite.h
 *
 *  Created on: Jul 7, 2021
 *      Author: popolony2k
 */

#ifndef __SPRITE_H__
#define __SPRITE_H__

#include "collider.h"
#include "drawentity.h"
#include "texturemap.h"
#include <map>


class Sprite : public DrawEntity  {

    typedef std :: map<int, TextureMap>  TextureSequenceList;

    Collider                        m_Collider;
    TextureSequenceList             m_Sequences;
    TextureSequenceList :: iterator m_itSequence;
    bool                            m_bIsValidSequence;

    public:

    Sprite( void );
    virtual ~Sprite( void );

    bool AddSpriteSequence( int nSequence,
                            std :: string strTextureFile,
                            stDimension2D dimension,
                            int64_t nDelayMilli = -1 );
    bool SetActiveSequence( int nSequence );

    void Move( stCoordinate2D& step );

    void Update( void );
};

#endif /* __SPRITE_H__ */
